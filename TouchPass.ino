// TouchPass - Hardware Password Manager
// A hardware password manager that types your passwords when you touch it with an enrolled finger
// ESP32-C6 with R502-A Fingerprint Sensor + BLE Keyboard
//
// Wiring:
//   R502-A Pin 1 (VCC)  -> 3V3
//   R502-A Pin 2 (GND)  -> GND
//   R502-A Pin 3 (TXD)  -> D7 (GPIO17) - ESP32 RX
//   R502-A Pin 4 (RXD)  -> D6 (GPIO16) - ESP32 TX
//   R502-A Pin 5 (IRQ)  -> D2 (GPIO4)  - Optional finger detect
//   R502-A Pin 6 (VT)   -> 3V3

#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>
#include <Preferences.h>
#include <BleKeyboard.h>
#include "webpage.h"

// Pin definitions for XIAO ESP32-C6
#define FP_TX_PIN 16  // D6 - ESP32 TX -> R502 RX
#define FP_RX_PIN 17  // D7 - ESP32 RX -> R502 TX

// R502-A Protocol constants
#define FP_HEADER 0xEF01
#define FP_DEFAULT_ADDR 0xFFFFFFFF
#define FP_CMD_PACKET 0x01

// R502-A Commands (only the ones we use)
#define CMD_GENIMG 0x01
#define CMD_IMG2TZ 0x02
#define CMD_SEARCH 0x04
#define CMD_REGMODEL 0x05
#define CMD_STORE 0x06
#define CMD_DELETCHAR 0x0C
#define CMD_EMPTY 0x0D
#define CMD_READSYSPARA 0x0F
#define CMD_TEMPLATENUM 0x1D
#define CMD_READINDEXTABLE 0x1F
#define CMD_AURALEDCONFIG 0x35
#define CMD_CHECKSENSOR 0x36
#define CMD_HANDSHAKE 0x40

// LED Colors (sensor LED, not ESP32 LED)
#define LED_RED 0x01
#define LED_BLUE 0x02
#define LED_GREEN 0x04
#define LED_CYAN 0x06

// LED Control modes
#define LED_BREATHING 0x01
#define LED_FLASHING 0x02
#define LED_ON 0x03
#define LED_OFF 0x04

// WiFi AP settings
const char* AP_SSID = "TouchPass";
const char* AP_PASS = "touchpass";

// Default Bluetooth device name
#define DEFAULT_BT_NAME "TouchPass"

// Create hardware serial for fingerprint sensor
HardwareSerial fpSerial(1);

// BLE Keyboard
BleKeyboard bleKeyboard(DEFAULT_BT_NAME, "Anthropic", 100);

// Web server
WebServer server(80);

// Preferences for storing finger names
Preferences prefs;

// Fingerprint module state
uint32_t fpAddress = FP_DEFAULT_ADDR;
uint16_t templateCount = 0;
uint16_t librarySize = 200;
String lastStatus = "Ready";
bool sensorOk = false;

// Enroll state machine - 6 capture Touch ID style enrollment
enum EnrollState {
    ENROLL_IDLE,
    ENROLL_CAPTURE_1,      // Waiting for capture 1 (center)
    ENROLL_LIFT_1,         // Waiting to lift finger
    ENROLL_CAPTURE_2,      // Waiting for capture 2 (center)
    ENROLL_LIFT_2,
    ENROLL_CAPTURE_3,      // Waiting for capture 3 (center)
    ENROLL_LIFT_3,
    ENROLL_CAPTURE_4,      // Waiting for capture 4 (edges)
    ENROLL_LIFT_4,
    ENROLL_CAPTURE_5,      // Waiting for capture 5 (edges)
    ENROLL_LIFT_5,
    ENROLL_CAPTURE_6,      // Waiting for capture 6 (edges)
    ENROLL_MERGING,        // Creating template from all captures
    ENROLL_DONE
};
EnrollState enrollState = ENROLL_IDLE;
String pendingFingerName = "";
String pendingFingerPassword = "";
bool pendingPressEnter = false;
int16_t pendingSlot = -1;
int pendingFingerId = -1;
bool enrollSuccess = false;
String enrollError = "";
unsigned long enrollTimeout = 0;

// Enrollment instruction messages (simple style)
const char* enrollMessages[] = {
    "Place finger on sensor",     // Capture 1
    "Lift and place again",       // Capture 2
    "Again, adjust slightly",     // Capture 3
    "Now adjust your grip",       // Capture 4 - Phase 2 (edges)
    "Place again",                // Capture 5
    "One more time"               // Capture 6
};

// Detection state for web UI
String lastDetectedFinger = "";
int lastDetectedId = -1;
int lastDetectedScore = 0;
uint8_t lastDetectResult = 0xFF;
bool newDetectionAvailable = false;

// WiFi state (off by default, toggle with long touch)
bool wifiEnabled = false;
#define LONG_TOUCH_MS 5000  // 5 seconds for config mode toggle

// Buffer for serial communication
uint8_t rxBuffer[256];

// Send command to fingerprint module
uint16_t sendCommand(uint8_t cmd, uint8_t* data, uint16_t dataLen) {
    uint16_t len = dataLen + 3; // cmd + checksum(2)
    uint16_t sum = FP_CMD_PACKET + (len >> 8) + (len & 0xFF) + cmd;

    // Header
    fpSerial.write((FP_HEADER >> 8) & 0xFF);
    fpSerial.write(FP_HEADER & 0xFF);

    // Address
    fpSerial.write((fpAddress >> 24) & 0xFF);
    fpSerial.write((fpAddress >> 16) & 0xFF);
    fpSerial.write((fpAddress >> 8) & 0xFF);
    fpSerial.write(fpAddress & 0xFF);

    // Packet type
    fpSerial.write(FP_CMD_PACKET);

    // Length
    fpSerial.write((len >> 8) & 0xFF);
    fpSerial.write(len & 0xFF);

    // Command
    fpSerial.write(cmd);

    // Data
    for (uint16_t i = 0; i < dataLen; i++) {
        fpSerial.write(data[i]);
        sum += data[i];
    }

    // Checksum
    fpSerial.write((sum >> 8) & 0xFF);
    fpSerial.write(sum & 0xFF);

    return len + 9; // Total bytes sent
}

// Receive response from fingerprint module
int16_t receiveResponse(uint8_t* buffer, uint16_t timeout) {
    unsigned long start = millis();
    uint16_t idx = 0;
    uint16_t packetLen = 0;
    bool headerFound = false;

    while (millis() - start < timeout) {
        if (fpSerial.available()) {
            uint8_t b = fpSerial.read();
            buffer[idx++] = b;

            // Check for header
            if (idx == 2) {
                if (buffer[0] == 0xEF && buffer[1] == 0x01) {
                    headerFound = true;
                } else {
                    // Shift buffer
                    buffer[0] = buffer[1];
                    idx = 1;
                }
            }

            // Get packet length
            if (headerFound && idx == 9) {
                packetLen = (buffer[7] << 8) | buffer[8];
            }

            // Check if complete
            if (headerFound && idx >= 9 && idx >= 9 + packetLen) {
                return idx;
            }

            if (idx >= sizeof(rxBuffer) - 1) {
                return -1; // Buffer overflow
            }
        }
    }

    return headerFound ? idx : -1;
}

// Get confirmation code from response
uint8_t getConfirmCode(uint8_t* buffer, int16_t len) {
    if (len < 10) return 0xFF;
    return buffer[9];
}

// Check sensor connection
bool checkSensorConnection() {
    sendCommand(CMD_HANDSHAKE, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 500);
    if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
        sensorOk = true;
        return true;
    }

    // Try checksensor command
    sendCommand(CMD_CHECKSENSOR, NULL, 0);
    len = receiveResponse(rxBuffer, 500);
    sensorOk = (len > 0 && getConfirmCode(rxBuffer, len) == 0x00);
    return sensorOk;
}

// Get template count
uint16_t getTemplateCount() {
    sendCommand(CMD_TEMPLATENUM, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 500);
    if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
        templateCount = (rxBuffer[10] << 8) | rxBuffer[11];
        return templateCount;
    }
    return 0;
}

// Read system parameters
bool readSysParams() {
    sendCommand(CMD_READSYSPARA, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 500);
    if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
        librarySize = (rxBuffer[14] << 8) | rxBuffer[15];
        return true;
    }
    return false;
}

// Control LED
bool setLED(uint8_t mode, uint8_t speed, uint8_t color, uint8_t count) {
    uint8_t data[4] = {mode, speed, color, count};
    sendCommand(CMD_AURALEDCONFIG, data, 4);
    int16_t len = receiveResponse(rxBuffer, 500);
    return (len > 0 && getConfirmCode(rxBuffer, len) == 0x00);
}

// Capture fingerprint image
uint8_t captureImage() {
    sendCommand(CMD_GENIMG, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 3000);
    if (len > 0) {
        return getConfirmCode(rxBuffer, len);
    }
    return 0xFF;
}

// Generate character file from image
uint8_t generateChar(uint8_t bufferId) {
    uint8_t data[1] = {bufferId};
    sendCommand(CMD_IMG2TZ, data, 1);
    int16_t len = receiveResponse(rxBuffer, 2000);
    if (len > 0) {
        return getConfirmCode(rxBuffer, len);
    }
    return 0xFF;
}

// Create template from character files
uint8_t createTemplate() {
    sendCommand(CMD_REGMODEL, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 2000);
    if (len > 0) {
        return getConfirmCode(rxBuffer, len);
    }
    return 0xFF;
}

// Store template
uint8_t storeTemplate(uint8_t bufferId, uint16_t id) {
    uint8_t data[3] = {bufferId, (uint8_t)(id >> 8), (uint8_t)(id & 0xFF)};
    sendCommand(CMD_STORE, data, 3);
    int16_t len = receiveResponse(rxBuffer, 2000);
    if (len > 0) {
        return getConfirmCode(rxBuffer, len);
    }
    return 0xFF;
}

// Search fingerprint
uint8_t searchFingerprint(uint8_t bufferId, uint16_t startId, uint16_t count, uint16_t* matchId, uint16_t* score) {
    uint8_t data[5] = {bufferId, (uint8_t)(startId >> 8), (uint8_t)(startId & 0xFF),
                       (uint8_t)(count >> 8), (uint8_t)(count & 0xFF)};
    sendCommand(CMD_SEARCH, data, 5);
    int16_t len = receiveResponse(rxBuffer, 3000);
    if (len > 0) {
        uint8_t code = getConfirmCode(rxBuffer, len);
        if (code == 0x00) {
            *matchId = (rxBuffer[10] << 8) | rxBuffer[11];
            *score = (rxBuffer[12] << 8) | rxBuffer[13];
        }
        return code;
    }
    return 0xFF;
}

// Delete template
uint8_t deleteTemplate(uint16_t id, uint16_t count) {
    uint8_t data[4] = {(uint8_t)(id >> 8), (uint8_t)(id & 0xFF),
                       (uint8_t)(count >> 8), (uint8_t)(count & 0xFF)};
    sendCommand(CMD_DELETCHAR, data, 4);
    int16_t len = receiveResponse(rxBuffer, 2000);
    if (len > 0) {
        return getConfirmCode(rxBuffer, len);
    }
    return 0xFF;
}

// Empty library
uint8_t emptyLibrary() {
    sendCommand(CMD_EMPTY, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 3000);
    if (len > 0) {
        return getConfirmCode(rxBuffer, len);
    }
    return 0xFF;
}

// Find first empty slot
int16_t findEmptySlot() {
    for (int page = 0; page < 1; page++) { // Just check first page (0-255)
        uint8_t data[1] = {(uint8_t)page};
        sendCommand(CMD_READINDEXTABLE, data, 1);
        int16_t len = receiveResponse(rxBuffer, 1000);
        if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
            // Parse index table - each bit represents a slot
            for (int i = 0; i < 32; i++) {
                uint8_t byte = rxBuffer[10 + i];
                for (int bit = 0; bit < 8; bit++) {
                    if (!(byte & (1 << bit))) {
                        return page * 256 + i * 8 + bit;
                    }
                }
            }
        }
    }
    return -1; // No empty slot
}

// Find slot for a specific finger ID (0-9)
int16_t findSlotForFinger(int fingerId) {
    if (fingerId < 0 || fingerId > 9) return -1;
    prefs.begin("fingers", true);
    for (int slot = 0; slot < librarySize; slot++) {
        String key = "i" + String(slot);
        if (prefs.isKey(key.c_str())) {
            if (prefs.getInt(key.c_str(), -1) == fingerId) {
                prefs.end();
                return slot;
            }
        }
    }
    prefs.end();
    return -1; // Not found
}

// Finger name storage functions
void saveFingerName(uint16_t id, String name) {
    prefs.begin("fingers", false);
    prefs.putString(("f" + String(id)).c_str(), name);
    // Also store the finger ID (0-9) mapping
    if (pendingFingerId >= 0 && pendingFingerId <= 9) {
        prefs.putInt(("i" + String(id)).c_str(), pendingFingerId);
    }
    prefs.end();
}

String getFingerName(uint16_t id) {
    prefs.begin("fingers", true);
    String name = prefs.getString(("f" + String(id)).c_str(), "");
    prefs.end();
    return name.length() > 0 ? name : "Finger " + String(id);
}

void deleteFingerName(uint16_t id) {
    prefs.begin("fingers", false);
    prefs.remove(("f" + String(id)).c_str());
    prefs.remove(("p" + String(id)).c_str());
    prefs.remove(("e" + String(id)).c_str());
    prefs.remove(("i" + String(id)).c_str()); // Remove finger ID mapping
    prefs.end();
}

void clearAllFingerNames() {
    prefs.begin("fingers", false);
    prefs.clear();
    prefs.end();
}

// Password storage functions
void saveFingerPassword(uint16_t id, String password) {
    prefs.begin("fingers", false);
    prefs.putString(("p" + String(id)).c_str(), password);
    prefs.end();
}

String getFingerPassword(uint16_t id) {
    prefs.begin("fingers", true);
    String pwd = prefs.getString(("p" + String(id)).c_str(), "");
    prefs.end();
    return pwd;
}

void saveFingerPressEnter(uint16_t id, bool pressEnter) {
    prefs.begin("fingers", false);
    prefs.putBool(("e" + String(id)).c_str(), pressEnter);
    prefs.end();
}

bool getFingerPressEnter(uint16_t id) {
    prefs.begin("fingers", true);
    bool pe = prefs.getBool(("e" + String(id)).c_str(), false);
    prefs.end();
    return pe;
}

// Type password via BLE keyboard
void typePassword(uint16_t fingerId) {
    String pwd = getFingerPassword(fingerId);
    if (pwd.length() == 0) {
        Serial.printf("[BLE] No password stored for finger %d\n", fingerId);
        return;
    }

    if (!bleKeyboard.isConnected()) {
        Serial.println("[BLE] Not connected - cannot type password");
        return;
    }

    Serial.printf("[BLE] Typing password for finger %d (%d chars)\n", fingerId, pwd.length());
    bleKeyboard.releaseAll();
    delay(50);

    // Type each character with small delay
    for (unsigned int i = 0; i < pwd.length(); i++) {
        bleKeyboard.print(String(pwd[i]));
        delay(30);
    }

    // Press Enter if configured
    if (getFingerPressEnter(fingerId)) {
        delay(50);
        bleKeyboard.write(KEY_RETURN);
        Serial.println("[BLE] Pressed Enter");
    }

    bleKeyboard.releaseAll();
    Serial.println("[BLE] Password typed");
}

// Set LED off (idle state)
void setIdleLED() {
    if (wifiEnabled) {
        setLED(LED_BREATHING, 100, LED_BLUE, 0);  // Keep blue breathing when WiFi on
    } else {
        setLED(LED_OFF, 0, LED_BLUE, 0);
    }
}

// WiFi control functions
void startWifi() {
    Serial.println("[WIFI] Starting AP...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    server.begin();
    wifiEnabled = true;
    setLED(LED_BREATHING, 100, LED_BLUE, 0);
    Serial.printf("[WIFI] AP started: %s (IP: %s)\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void stopWifi() {
    Serial.println("[WIFI] Stopping AP...");
    server.close();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    wifiEnabled = false;
    setLED(LED_OFF, 0, LED_BLUE, 0);
    Serial.println("[WIFI] AP stopped");
}

void toggleWifi() {
    if (wifiEnabled) {
        stopWifi();
    } else {
        startWifi();
    }
}

// Handle automatic finger detection (called from loop)
void processFingerDetection() {
    // Skip if in enroll mode
    if (enrollState != ENROLL_IDLE) return;

    // Rate limit detection attempts
    static unsigned long lastAttempt = 0;
    if (millis() - lastAttempt < 500) return;
    lastAttempt = millis();

    // Check if finger is on sensor
    uint8_t result = captureImage();
    if (result != 0x00) return;  // No finger

    // Finger detected! Immediately try to identify
    Serial.println("[DETECT] Finger detected, identifying...");

    result = generateChar(1);
    if (result != 0x00) {
        Serial.printf("[DETECT] Feature extraction failed: 0x%02X\n", result);
        lastDetectResult = result;
        newDetectionAvailable = true;
        lastStatus = "Unknown finger";
        setLED(LED_ON, 0, LED_RED, 0);
        delay(2000);
        setIdleLED();
        // Wait for finger removal
        while (captureImage() == 0x00) delay(200);
        return;
    }

    // Search in database
    uint16_t matchId = 0, score = 0;
    result = searchFingerprint(1, 0, librarySize, &matchId, &score);
    Serial.printf("[DETECT] searchFingerprint: 0x%02X, id=%d, score=%d\n", result, matchId, score);

    if (result == 0x00) {
        // Match found!
        lastDetectedFinger = getFingerName(matchId);
        lastDetectedId = matchId;
        lastDetectedScore = score;
        lastDetectResult = 0x00;
        newDetectionAvailable = true;
        lastStatus = lastDetectedFinger + " detected";
        Serial.printf("[DETECT] *** MATCH: %s (ID:%d, Score:%d) ***\n",
                      lastDetectedFinger.c_str(), matchId, score);
        setLED(LED_ON, 0, LED_GREEN, 0);

        // Type password via BLE if connected
        typePassword(matchId);

        // Now check if user keeps holding for settings mode
        Serial.println("[DETECT] Password typed. Hold 5s more for settings...");
        unsigned long holdStart = millis();
        bool enteredSettings = false;

        while (millis() - holdStart < LONG_TOUCH_MS) {
            delay(200);
            if (captureImage() != 0x00) {
                // Finger removed
                Serial.println("[DETECT] Finger removed");
                break;
            }
            if (millis() - holdStart >= LONG_TOUCH_MS) {
                // Still holding after 5 seconds - toggle WiFi
                Serial.println("[DETECT] Long hold - toggling WiFi");
                toggleWifi();
                enteredSettings = true;
                break;
            }
        }

        if (!enteredSettings) {
            delay(1000);  // Brief pause after password typed
        }
        setIdleLED();

        // Wait for finger removal
        while (captureImage() == 0x00) delay(200);

    } else {
        // No match - but still check for long hold to enter settings
        Serial.printf("[DETECT] No match (code: 0x%02X) - hold 5s for settings\n", result);
        lastDetectedFinger = "";
        lastDetectedId = -1;
        lastDetectedScore = 0;
        lastDetectResult = result;
        newDetectionAvailable = true;
        lastStatus = "Unknown finger";
        setLED(LED_ON, 0, LED_RED, 0);

        unsigned long holdStart = millis();
        while (millis() - holdStart < LONG_TOUCH_MS) {
            delay(200);
            if (captureImage() != 0x00) {
                // Finger removed
                break;
            }
            if (millis() - holdStart >= LONG_TOUCH_MS) {
                // Still holding after 5 seconds - toggle WiFi
                Serial.println("[DETECT] Long hold - toggling WiFi");
                toggleWifi();
                break;
            }
        }

        setIdleLED();

        // Wait for finger removal
        while (captureImage() == 0x00) delay(200);
    }
}

// Web handlers
void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

void handleStatus() {
    getTemplateCount();
    String json = "{";
    json += "\"sensor\":" + String(sensorOk ? "true" : "false") + ",";
    json += "\"count\":" + String(templateCount) + ",";
    json += "\"capacity\":" + String(librarySize) + ",";
    json += "\"last\":\"" + lastStatus + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

// Poll for finger detection events
void handleDetect() {
    String json = "{";
    json += "\"detected\":" + String(newDetectionAvailable ? "true" : "false") + ",";
    json += "\"finger\":\"" + lastDetectedFinger + "\",";
    json += "\"id\":" + String(lastDetectedId) + ",";
    json += "\"score\":" + String(lastDetectedScore) + ",";
    json += "\"matched\":" + String(lastDetectedId >= 0 ? "true" : "false") + ",";
    json += "\"lastResult\":\"0x" + String(lastDetectResult, HEX) + "\"";
    json += "}";

    // Clear the flag after reading
    if (newDetectionAvailable) {
        newDetectionAvailable = false;
    }

    server.send(200, "application/json", json);
}

// Get finger ID (0-9) for a slot
int getFingerIdForSlot(uint16_t slot) {
    prefs.begin("fingers", true);
    String key = "i" + String(slot);
    int fingerId = prefs.getInt(key.c_str(), -1);
    prefs.end();
    return fingerId;
}

// Get list of enrolled fingers with names
void handleFingers() {
    String json = "{\"fingers\":[";
    bool first = true;

    for (int page = 0; page < 1; page++) {
        uint8_t data[1] = {(uint8_t)page};
        sendCommand(CMD_READINDEXTABLE, data, 1);
        int16_t len = receiveResponse(rxBuffer, 1000);
        if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
            for (int i = 0; i < 32; i++) {
                uint8_t byte = rxBuffer[10 + i];
                for (int bit = 0; bit < 8; bit++) {
                    if (byte & (1 << bit)) {
                        int id = page * 256 + i * 8 + bit;
                        if (id < librarySize) {
                            if (!first) json += ",";
                            first = false;
                            String name = getFingerName(id);
                            int fingerId = getFingerIdForSlot(id);
                            json += "{\"id\":" + String(id) + ",\"name\":\"" + name + "\",\"fingerId\":" + String(fingerId) + "}";
                        }
                    }
                }
            }
        }
    }

    json += "]}";
    server.send(200, "application/json", json);
}

// Start enrollment process
void handleEnrollStart() {
    if (!server.hasArg("name")) {
        server.send(400, "application/json", "{\"ok\":false,\"status\":\"Missing name\"}");
        return;
    }

    pendingFingerName = server.arg("name");
    pendingFingerPassword = server.hasArg("password") ? server.arg("password") : "";
    pendingPressEnter = server.hasArg("pressEnter") && server.arg("pressEnter") == "true";
    pendingFingerId = server.hasArg("finger") ? server.arg("finger").toInt() : -1;

    // Check if this finger already has an enrollment - delete it first
    int16_t existingSlot = findSlotForFinger(pendingFingerId);
    if (existingSlot >= 0) {
        Serial.printf("[DEBUG] Finger %d already enrolled at slot %d, deleting...\n", pendingFingerId, existingSlot);
        deleteTemplate(existingSlot, 1);
        deleteFingerName(existingSlot);
    }

    pendingSlot = findEmptySlot();

    if (pendingSlot < 0 || pendingSlot >= librarySize) {
        server.send(200, "application/json", "{\"ok\":false,\"status\":\"Library full\"}");
        return;
    }

    enrollState = ENROLL_CAPTURE_1;
    enrollSuccess = false;
    enrollError = "";
    enrollTimeout = millis() + 60000; // 60 second timeout for 6 captures
    setLED(LED_BREATHING, 100, LED_BLUE, 0);
    lastStatus = "Enrolling " + pendingFingerName;
    Serial.printf("[DEBUG] Starting 6-capture enrollment for: %s, slot: %d, password: %s\n",
                  pendingFingerName.c_str(), pendingSlot,
                  pendingFingerPassword.length() > 0 ? "(set)" : "(none)");

    server.send(200, "application/json", "{\"ok\":true,\"status\":\"Place finger on sensor\"}");
}

// Poll enrollment status - returns detailed progress for 6-capture flow
void handleEnrollStatus() {
    String json = "{";
    json += "\"state\":" + String(enrollState) + ",";
    json += "\"totalSteps\":6,";

    // Map state to step number and captured status
    int step = 0;
    bool captured = false;
    bool done = false;
    const char* phase = "center";
    const char* message = "";

    switch (enrollState) {
        case ENROLL_IDLE:
            step = 0;
            break;
        case ENROLL_CAPTURE_1:
            step = 1;
            message = enrollMessages[0];
            break;
        case ENROLL_LIFT_1:
            step = 1;
            captured = true;
            message = enrollMessages[1];
            break;
        case ENROLL_CAPTURE_2:
            step = 2;
            message = enrollMessages[1];
            break;
        case ENROLL_LIFT_2:
            step = 2;
            captured = true;
            message = enrollMessages[2];
            break;
        case ENROLL_CAPTURE_3:
            step = 3;
            message = enrollMessages[2];
            break;
        case ENROLL_LIFT_3:
            step = 3;
            captured = true;
            phase = "edges";
            message = enrollMessages[3];
            break;
        case ENROLL_CAPTURE_4:
            step = 4;
            phase = "edges";
            message = enrollMessages[3];
            break;
        case ENROLL_LIFT_4:
            step = 4;
            captured = true;
            phase = "edges";
            message = enrollMessages[4];
            break;
        case ENROLL_CAPTURE_5:
            step = 5;
            phase = "edges";
            message = enrollMessages[4];
            break;
        case ENROLL_LIFT_5:
            step = 5;
            captured = true;
            phase = "edges";
            message = enrollMessages[5];
            break;
        case ENROLL_CAPTURE_6:
            step = 6;
            phase = "edges";
            message = enrollMessages[5];
            break;
        case ENROLL_MERGING:
            step = 6;
            captured = true;
            phase = "edges";
            message = "Creating template...";
            break;
        case ENROLL_DONE:
            step = 6;
            captured = true;
            done = true;
            phase = "edges";
            break;
    }

    json += "\"step\":" + String(step) + ",";
    json += "\"captured\":" + String(captured ? "true" : "false") + ",";
    json += "\"phase\":\"" + String(phase) + "\",";
    json += "\"message\":\"" + String(message) + "\",";
    json += "\"done\":" + String(done ? "true" : "false");

    if (enrollState == ENROLL_DONE) {
        json += ",\"ok\":" + String(enrollSuccess ? "true" : "false");
        json += ",\"name\":\"" + pendingFingerName + "\"";
        json += ",\"status\":\"" + (enrollSuccess ? String("Enrolled successfully") : enrollError) + "\"";
        enrollState = ENROLL_IDLE;
        if (!enrollSuccess) {
            setLED(LED_ON, 0, LED_RED, 0);
            delay(100);
        }
        setIdleLED();
    }

    json += "}";
    server.send(200, "application/json", json);
}

// Cancel enrollment
void handleEnrollCancel() {
    enrollState = ENROLL_IDLE;
    pendingFingerName = "";
    pendingFingerPassword = "";
    pendingPressEnter = false;
    pendingSlot = -1;
    setIdleLED();
    server.send(200, "application/json", "{\"ok\":true}");
}

// Helper: Try to capture and generate feature into specified buffer
// Returns true on success, false on failure (sets enrollError)
bool enrollCaptureToBuffer(uint8_t bufferNum) {
    uint8_t result = captureImage();
    if (result != 0x00) return false;  // No finger yet

    result = generateChar(bufferNum);
    if (result != 0x00) {
        enrollError = "Feature extraction failed";
        enrollState = ENROLL_DONE;
        enrollSuccess = false;
        setLED(LED_ON, 0, LED_RED, 0);
        return false;
    }
    return true;
}

// Helper: Check if finger is lifted
bool isFingerLifted() {
    return captureImage() == 0x02;  // 0x02 = no finger detected
}

// Process enrollment state machine (called from loop)
// 6-capture Touch ID style enrollment using CharBuffer 1-6
void processEnrollment() {
    if (enrollState == ENROLL_IDLE) return;

    // Check timeout
    if (millis() > enrollTimeout) {
        enrollState = ENROLL_DONE;
        enrollSuccess = false;
        enrollError = "Timeout";
        setLED(LED_ON, 0, LED_RED, 0);
        return;
    }

    uint8_t result;

    switch (enrollState) {
        // === CAPTURE 1 (Center) ===
        case ENROLL_CAPTURE_1:
            if (enrollCaptureToBuffer(1)) {
                Serial.println("[ENROLL] Capture 1 success");
                enrollState = ENROLL_LIFT_1;
                setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_1:
            if (isFingerLifted()) {
                enrollState = ENROLL_CAPTURE_2;
                setLED(LED_BREATHING, 100, LED_BLUE, 0);
            }
            break;

        // === CAPTURE 2 (Center) ===
        case ENROLL_CAPTURE_2:
            if (enrollCaptureToBuffer(2)) {
                Serial.println("[ENROLL] Capture 2 success");
                enrollState = ENROLL_LIFT_2;
                setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_2:
            if (isFingerLifted()) {
                enrollState = ENROLL_CAPTURE_3;
                setLED(LED_BREATHING, 100, LED_BLUE, 0);
            }
            break;

        // === CAPTURE 3 (Center) ===
        case ENROLL_CAPTURE_3:
            if (enrollCaptureToBuffer(3)) {
                Serial.println("[ENROLL] Capture 3 success");
                enrollState = ENROLL_LIFT_3;
                setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_3:
            if (isFingerLifted()) {
                // Phase 2 starts - flash cyan to indicate grip change
                enrollState = ENROLL_CAPTURE_4;
                setLED(LED_FLASHING, 100, LED_CYAN, 3);
                delay(300);
                setLED(LED_BREATHING, 100, LED_BLUE, 0);
                Serial.println("[ENROLL] Phase 2: Edges");
            }
            break;

        // === CAPTURE 4 (Edges) ===
        case ENROLL_CAPTURE_4:
            if (enrollCaptureToBuffer(4)) {
                Serial.println("[ENROLL] Capture 4 success");
                enrollState = ENROLL_LIFT_4;
                setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_4:
            if (isFingerLifted()) {
                enrollState = ENROLL_CAPTURE_5;
                setLED(LED_BREATHING, 100, LED_BLUE, 0);
            }
            break;

        // === CAPTURE 5 (Edges) ===
        case ENROLL_CAPTURE_5:
            if (enrollCaptureToBuffer(5)) {
                Serial.println("[ENROLL] Capture 5 success");
                enrollState = ENROLL_LIFT_5;
                setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_5:
            if (isFingerLifted()) {
                enrollState = ENROLL_CAPTURE_6;
                setLED(LED_BREATHING, 100, LED_BLUE, 0);
            }
            break;

        // === CAPTURE 6 (Edges) ===
        case ENROLL_CAPTURE_6:
            if (enrollCaptureToBuffer(6)) {
                Serial.println("[ENROLL] Capture 6 success - merging template");
                enrollState = ENROLL_MERGING;
                setLED(LED_BREATHING, 50, LED_CYAN, 0);
            }
            break;

        // === MERGE ALL 6 CAPTURES INTO TEMPLATE ===
        case ENROLL_MERGING:
            // Create template from all 6 CharBuffers
            result = createTemplate();
            if (result != 0x00) {
                Serial.printf("[ENROLL] Template merge failed: 0x%02X\n", result);
                enrollError = "Template merge failed";
                enrollState = ENROLL_DONE;
                enrollSuccess = false;
                setLED(LED_ON, 0, LED_RED, 0);
                return;
            }

            // Store template (from buffer 1 which now has merged template)
            result = storeTemplate(1, pendingSlot);
            if (result != 0x00) {
                Serial.printf("[ENROLL] Store failed: 0x%02X\n", result);
                enrollError = "Store failed";
                enrollState = ENROLL_DONE;
                enrollSuccess = false;
                setLED(LED_ON, 0, LED_RED, 0);
                return;
            }

            // Success!
            Serial.printf("[ENROLL] Success! Stored at slot %d\n", pendingSlot);
            saveFingerName(pendingSlot, pendingFingerName);
            if (pendingFingerPassword.length() > 0) {
                saveFingerPassword(pendingSlot, pendingFingerPassword);
                saveFingerPressEnter(pendingSlot, pendingPressEnter);
                Serial.printf("[DEBUG] Saved password for slot %d\n", pendingSlot);
            }
            getTemplateCount();
            enrollState = ENROLL_DONE;
            enrollSuccess = true;
            setLED(LED_ON, 0, LED_GREEN, 0);
            delay(500);
            lastStatus = pendingFingerName + " enrolled";
            // Clear pending password from memory
            pendingFingerPassword = "";
            break;

        default:
            break;
    }
}

void handleDelete() {
    if (!server.hasArg("id")) {
        server.send(400, "application/json", "{\"ok\":false,\"status\":\"Missing ID\"}");
        return;
    }

    int id = server.arg("id").toInt();
    if (id < 0 || id >= librarySize) {
        server.send(400, "application/json", "{\"ok\":false,\"status\":\"Invalid ID\"}");
        return;
    }

    String name = getFingerName(id);
    uint8_t result = deleteTemplate(id, 1);
    getTemplateCount();

    if (result == 0x00) {
        deleteFingerName(id);
        setLED(LED_ON, 0, LED_GREEN, 0);
        delay(500);
        setIdleLED();
        lastStatus = "Deleted " + name;
        server.send(200, "application/json", "{\"ok\":true,\"status\":\"Deleted " + name + "\",\"count\":" + String(templateCount) + "}");
    } else {
        lastStatus = "Delete failed";
        server.send(200, "application/json", "{\"ok\":false,\"status\":\"Delete failed\",\"count\":" + String(templateCount) + "}");
    }
}

void handleEmpty() {
    uint8_t result = emptyLibrary();
    getTemplateCount();

    if (result == 0x00) {
        clearAllFingerNames();
        setLED(LED_ON, 0, LED_GREEN, 0);
        delay(500);
        setIdleLED();
        lastStatus = "Library cleared";
        server.send(200, "application/json", "{\"ok\":true,\"status\":\"All fingerprints deleted\",\"count\":" + String(templateCount) + "}");
    } else {
        lastStatus = "Clear failed";
        server.send(200, "application/json", "{\"ok\":false,\"status\":\"Failed to clear library\",\"count\":" + String(templateCount) + "}");
    }
}

// Get BLE connection status
void handleBleStatus() {
    bool connected = bleKeyboard.isConnected();
    String json = "{\"connected\":" + String(connected ? "true" : "false") + "}";
    server.send(200, "application/json", json);
}

// Get finger details including password info
void handleFingerGet() {
    if (!server.hasArg("id")) {
        server.send(400, "application/json", "{\"ok\":false,\"status\":\"Missing ID\"}");
        return;
    }

    int id = server.arg("id").toInt();
    if (id < 0 || id >= librarySize) {
        server.send(400, "application/json", "{\"ok\":false,\"status\":\"Invalid ID\"}");
        return;
    }

    String name = getFingerName(id);
    String pwd = getFingerPassword(id);
    bool pressEnter = getFingerPressEnter(id);
    int fingerId = getFingerIdForSlot(id);

    String json = "{\"ok\":true,";
    json += "\"id\":" + String(id) + ",";
    json += "\"name\":\"" + name + "\",";
    json += "\"hasPassword\":" + String(pwd.length() > 0 ? "true" : "false") + ",";
    json += "\"pressEnter\":" + String(pressEnter ? "true" : "false") + ",";
    json += "\"fingerId\":" + String(fingerId);
    json += "}";
    server.send(200, "application/json", json);
}

// Update finger name/password/pressEnter
void handleFingerUpdate() {
    if (!server.hasArg("id")) {
        server.send(400, "application/json", "{\"ok\":false,\"status\":\"Missing ID\"}");
        return;
    }

    int id = server.arg("id").toInt();
    if (id < 0 || id >= librarySize) {
        server.send(400, "application/json", "{\"ok\":false,\"status\":\"Invalid ID\"}");
        return;
    }

    // Update name if provided
    if (server.hasArg("name")) {
        saveFingerName(id, server.arg("name"));
    }

    // Update password if provided
    if (server.hasArg("password")) {
        saveFingerPassword(id, server.arg("password"));
    }

    // Update pressEnter if provided
    if (server.hasArg("pressEnter")) {
        saveFingerPressEnter(id, server.arg("pressEnter") == "true");
    }

    String name = getFingerName(id);
    server.send(200, "application/json", "{\"ok\":true,\"status\":\"Updated " + name + "\"}");
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Fingerprint Password Manager ===");

    // Initialize fingerprint serial
    fpSerial.begin(57600, SERIAL_8N1, FP_RX_PIN, FP_TX_PIN);
    delay(500);

    // Clear any startup garbage
    while (fpSerial.available()) fpSerial.read();

    // Check sensor
    if (checkSensorConnection()) {
        Serial.println("Sensor connected!");
        readSysParams();
        getTemplateCount();
        Serial.printf("Library: %d/%d\n", templateCount, librarySize);
        setLED(LED_ON, 0, LED_GREEN, 0);
        delay(500);
        setIdleLED();
    } else {
        Serial.println("Sensor not responding!");
        setLED(LED_ON, 0, LED_RED, 0);
    }

    // Start BLE Keyboard
    bleKeyboard.begin();
    Serial.println("BLE started - pair with 'FP-PWD'");

    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/status", handleStatus);
    server.on("/detect", handleDetect);
    server.on("/fingers", handleFingers);
    server.on("/enroll/start", handleEnrollStart);
    server.on("/enroll/status", handleEnrollStatus);
    server.on("/enroll/cancel", handleEnrollCancel);
    server.on("/delete", handleDelete);
    server.on("/empty", handleEmpty);
    server.on("/ble/status", handleBleStatus);
    server.on("/finger/get", handleFingerGet);
    server.on("/finger/update", handleFingerUpdate);

    Serial.println("Ready! Hold finger 5s for WiFi config.");
}

void loop() {
    if (wifiEnabled) {
        server.handleClient();
        processEnrollment();
    }
    processFingerDetection();
}
