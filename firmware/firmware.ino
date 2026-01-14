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

#include <HardwareSerial.h>
#include <Preferences.h>
#include <BleKeyboard.h>
#include <ArduinoJson.h>
#include "SerialCommandHandler.h"
#include "sdkconfig.h"
#include "KeyboardInterface.h"

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

// Default Bluetooth device name
#define DEFAULT_BT_NAME "TouchPass"

// Chip detection and capabilities
#if CONFIG_IDF_TARGET_ESP32C6
  #define CHIP_TYPE "ESP32-C6"
  #define HAS_NATIVE_USB false
  #define DEFAULT_KB_MODE KB_MODE_BLE
#elif CONFIG_IDF_TARGET_ESP32S3
  #define CHIP_TYPE "ESP32-S3"
  #define HAS_NATIVE_USB true
  // Temporarily using BLE until USB keyboard implementation is fixed
  #define DEFAULT_KB_MODE KB_MODE_BLE
#else
  #error "Unsupported chip type - only ESP32-C6 and ESP32-S3 are supported"
#endif

// Create hardware serial for fingerprint sensor
HardwareSerial fpSerial(1);

// Keyboard interface (BLE or USB depending on chip and configuration)
KeyboardInterface* keyboard = nullptr;
KeyboardMode currentKeyboardMode = KB_MODE_AUTO;

// Serial command handler
SerialCommandHandler serialHandler;

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

// ========================================
// Keyboard Mode Configuration Functions
// ========================================

void saveKeyboardMode(KeyboardMode mode) {
    prefs.begin("fingers", false);
    prefs.putUChar("kbmode", (uint8_t)mode);
    prefs.end();
    Serial.printf("[CONFIG] Saved keyboard mode: %d\n", mode);
}

KeyboardMode loadKeyboardMode() {
    prefs.begin("fingers", true);
    uint8_t mode = prefs.getUChar("kbmode", KB_MODE_AUTO);
    prefs.end();
    return (KeyboardMode)mode;
}

String getKeyboardModeString(KeyboardMode mode) {
    switch(mode) {
        case KB_MODE_BLE: return "BLE";
        case KB_MODE_USB: return "USB-HID";
        case KB_MODE_AUTO: return "Auto";
        default: return "Unknown";
    }
}

bool initKeyboard() {
    KeyboardMode requestedMode = loadKeyboardMode();

    // Resolve AUTO mode to chip default
    if (requestedMode == KB_MODE_AUTO) {
        requestedMode = DEFAULT_KB_MODE;
    }

    // Validate mode for chip capabilities
    if (requestedMode == KB_MODE_USB && !HAS_NATIVE_USB) {
        Serial.println("[ERROR] USB mode not supported on ESP32-C6, falling back to BLE");
        requestedMode = KB_MODE_BLE;
    }

    // Initialize appropriate keyboard
    if (requestedMode == KB_MODE_BLE) {
        keyboard = new BLEKeyboardWrapper(DEFAULT_BT_NAME, "Anthropic", 100);
        Serial.println("[KEYBOARD] Initializing BLE keyboard");
    }
#if 0 && CONFIG_IDF_TARGET_ESP32S3
    else if (requestedMode == KB_MODE_USB) {
        keyboard = new USBKeyboardWrapper();
        Serial.println("[KEYBOARD] Initializing USB-HID keyboard");
    }
#endif
    else {
        Serial.println("[ERROR] Invalid keyboard mode");
        return false;
    }

    currentKeyboardMode = requestedMode;
    bool success = keyboard->begin();

    if (success) {
        Serial.printf("[KEYBOARD] Started in %s mode\n", keyboard->getModeName().c_str());
    } else {
        Serial.println("[ERROR] Keyboard initialization failed");
    }

    return success;
}

// Type password via keyboard (BLE or USB)
void typePassword(uint16_t fingerId) {
    String pwd = getFingerPassword(fingerId);
    if (pwd.length() == 0) {
        Serial.printf("[KEYBOARD] No password stored for finger %d\n", fingerId);
        return;
    }

    if (!keyboard || !keyboard->isConnected()) {
        Serial.println("[KEYBOARD] Not connected - cannot type password");
        return;
    }

    Serial.printf("[KEYBOARD] Typing password for finger %d (%d chars) via %s\n",
                  fingerId, pwd.length(), keyboard->getModeName().c_str());
    keyboard->releaseAll();
    delay(50);

    // Type password using keyboard interface
    keyboard->print(pwd);

    // Press Enter if configured
    if (getFingerPressEnter(fingerId)) {
        delay(50);
        keyboard->write(KEY_RETURN);
        Serial.println("[KEYBOARD] Pressed Enter");
    }

    keyboard->releaseAll();
    Serial.println("[KEYBOARD] Password typed");
}

// Set LED off (idle state)
void setIdleLED() {
    setLED(LED_OFF, 0, LED_BLUE, 0);
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

        // Brief pause after password typed
        delay(1000);
        setIdleLED();

        // Wait for finger removal
        while (captureImage() == 0x00) delay(200);

    } else {
        // No match
        Serial.printf("[DETECT] No match (code: 0x%02X)\n", result);
        lastDetectedFinger = "";
        lastDetectedId = -1;
        lastDetectedScore = 0;
        lastDetectResult = result;
        newDetectionAvailable = true;
        lastStatus = "Unknown finger";
        setLED(LED_ON, 0, LED_RED, 0);
        delay(2000);
        setIdleLED();

        // Wait for finger removal
        while (captureImage() == 0x00) delay(200);
    }
}

// JSON handler functions (called by SerialCommandHandler)

String getStatusJson() {
    getTemplateCount();
    String json = "{";
    json += "\"sensor\":" + String(sensorOk ? "true" : "false") + ",";
    json += "\"count\":" + String(templateCount) + ",";
    json += "\"capacity\":" + String(librarySize) + ",";
    json += "\"last\":\"" + lastStatus + "\"";
    json += "}";
    return json;
}

String getDetectJson() {
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

    return json;
}

// Get finger ID (0-9) for a slot
int getFingerIdForSlot(uint16_t slot) {
    prefs.begin("fingers", true);
    String key = "i" + String(slot);
    int fingerId = prefs.getInt(key.c_str(), -1);
    prefs.end();
    return fingerId;
}

String getFingersJson() {
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
    return json;
}

String enrollStartJson(JsonObject params) {
    const char* name = params["name"];
    if (!name) {
        return "{\"ok\":false,\"status\":\"Missing name\"}";
    }

    pendingFingerName = String(name);
    pendingFingerPassword = params.containsKey("password") ? String((const char*)params["password"]) : "";
    pendingPressEnter = params["pressEnter"] | false;
    pendingFingerId = params["finger"] | -1;

    // Check if this finger already has an enrollment - delete it first
    int16_t existingSlot = findSlotForFinger(pendingFingerId);
    if (existingSlot >= 0) {
        Serial.printf("[DEBUG] Finger %d already enrolled at slot %d, deleting...\n", pendingFingerId, existingSlot);
        deleteTemplate(existingSlot, 1);
        deleteFingerName(existingSlot);
    }

    pendingSlot = findEmptySlot();

    if (pendingSlot < 0 || pendingSlot >= librarySize) {
        return "{\"ok\":false,\"status\":\"Library full\"}";
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

    return "{\"ok\":true,\"status\":\"Place finger on sensor\"}";
}

String getEnrollStatusJson() {
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
    return json;
}

String enrollCancelJson() {
    enrollState = ENROLL_IDLE;
    pendingFingerName = "";
    pendingFingerPassword = "";
    pendingPressEnter = false;
    pendingSlot = -1;
    setIdleLED();
    return "{\"ok\":true}";
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

String deleteFingerJson(JsonObject params) {
    int id = params["id"] | -1;
    if (id < 0) {
        return "{\"ok\":false,\"status\":\"Missing ID\"}";
    }

    if (id >= librarySize) {
        return "{\"ok\":false,\"status\":\"Invalid ID\"}";
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
        return "{\"ok\":true,\"status\":\"Deleted " + name + "\",\"count\":" + String(templateCount) + "}";
    } else {
        lastStatus = "Delete failed";
        return "{\"ok\":false,\"status\":\"Delete failed\",\"count\":" + String(templateCount) + "}";
    }
}

String emptyLibraryJson() {
    uint8_t result = emptyLibrary();
    getTemplateCount();

    if (result == 0x00) {
        clearAllFingerNames();
        setLED(LED_ON, 0, LED_GREEN, 0);
        delay(500);
        setIdleLED();
        lastStatus = "Library cleared";
        return "{\"ok\":true,\"status\":\"All fingerprints deleted\",\"count\":" + String(templateCount) + "}";
    } else {
        lastStatus = "Clear failed";
        return "{\"ok\":false,\"status\":\"Failed to clear library\",\"count\":" + String(templateCount) + "}";
    }
}

String getBLEStatusJson() {
    bool connected = keyboard && keyboard->isConnected();
    String json = "{\"connected\":" + String(connected ? "true" : "false");
    json += ",\"mode\":\"" + (keyboard ? keyboard->getModeName() : "none") + "\"";
    json += "}";
    return json;
}

String getFingerJson(JsonObject params) {
    int id = params["id"] | -1;
    if (id < 0) {
        return "{\"ok\":false,\"status\":\"Missing ID\"}";
    }

    if (id >= librarySize) {
        return "{\"ok\":false,\"status\":\"Invalid ID\"}";
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
    return json;
}

String updateFingerJson(JsonObject params) {
    int id = params["id"] | -1;
    if (id < 0) {
        return "{\"ok\":false,\"status\":\"Missing ID\"}";
    }

    if (id >= librarySize) {
        return "{\"ok\":false,\"status\":\"Invalid ID\"}";
    }

    // Update name if provided
    if (params.containsKey("name")) {
        saveFingerName(id, String((const char*)params["name"]));
    }

    // Update password if provided
    if (params.containsKey("password")) {
        saveFingerPassword(id, String((const char*)params["password"]));
    }

    // Update pressEnter if provided
    if (params.containsKey("pressEnter")) {
        saveFingerPressEnter(id, params["pressEnter"] | false);
    }

    String name = getFingerName(id);
    return "{\"ok\":true,\"status\":\"Updated " + name + "\"}";
}

String getSystemInfoJson() {
    String json = "{";
    json += "\"chip\":\"" + String(CHIP_TYPE) + "\",";
    json += "\"chipModel\":\"" + String(ESP.getChipModel()) + "\",";
    json += "\"hasUSB\":" + String(HAS_NATIVE_USB ? "true" : "false") + ",";
    json += "\"keyboardMode\":\"" + (keyboard ? keyboard->getModeName() : "none") + "\",";
    json += "\"keyboardConnected\":" + String((keyboard && keyboard->isConnected()) ? "true" : "false");
    json += "}";
    return json;
}

String getKeyboardModeJson() {
    KeyboardMode savedMode = loadKeyboardMode();
    String json = "{";
    json += "\"current\":\"" + getKeyboardModeString(currentKeyboardMode) + "\",";
    json += "\"saved\":\"" + getKeyboardModeString(savedMode) + "\",";
    json += "\"connected\":" + String((keyboard && keyboard->isConnected()) ? "true" : "false") + ",";
    json += "\"chip\":\"" + String(CHIP_TYPE) + "\",";
    json += "\"hasUSB\":" + String(HAS_NATIVE_USB ? "true" : "false") + ",";
    json += "\"availableModes\":[\"BLE\"";
#if CONFIG_IDF_TARGET_ESP32S3
    json += ",\"USB-HID\"";
#endif
    json += "]";
    json += "}";
    return json;
}

String setKeyboardModeJson(JsonObject params) {
    const char* modeStr = params["mode"];
    if (!modeStr) {
        return "{\"ok\":false,\"error\":\"Missing mode parameter\"}";
    }

    String modeStrLower = String(modeStr);
    modeStrLower.toLowerCase();
    KeyboardMode newMode;

    if (modeStrLower == "ble") {
        newMode = KB_MODE_BLE;
    } else if (modeStrLower == "usb") {
        if (!HAS_NATIVE_USB) {
            return "{\"ok\":false,\"error\":\"USB not supported on this chip\"}";
        }
        newMode = KB_MODE_USB;
    } else if (modeStrLower == "auto") {
        newMode = KB_MODE_AUTO;
    } else {
        return "{\"ok\":false,\"error\":\"Invalid mode\"}";
    }

    saveKeyboardMode(newMode);

    String json = "{\"ok\":true,";
    json += "\"newMode\":\"" + getKeyboardModeString(newMode) + "\",";
    json += "\"message\":\"Mode saved. Reboot required.\"}";
    return json;
}

String rebootJson() {
    delay(500);
    ESP.restart();
    return "{\"ok\":true,\"message\":\"Rebooting...\"}";
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== TouchPass Fingerprint Password Manager ===");
    Serial.printf("Chip: %s (%s)\n", CHIP_TYPE, ESP.getChipModel());
    Serial.printf("USB Support: %s\n", HAS_NATIVE_USB ? "Yes" : "No");

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

    // Initialize keyboard
    if (!initKeyboard()) {
        Serial.println("Failed to initialize keyboard!");
    }

    // Initialize serial command handler
    serialHandler.begin();

    Serial.println("Ready! Connect via USB for configuration.");
}

void loop() {
    serialHandler.loop();       // Process serial commands
    processEnrollment();        // Handle enrollment (if active)
    processFingerDetection();   // Auto-detect fingers and type passwords
}
