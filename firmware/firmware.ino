// TouchPass - Hardware Password Manager
// ESP32-S3/C6 + R502-A Fingerprint Sensor + USB/BLE Keyboard
//
// Wiring (ESP32-S3):
//   Fingerprint: VCC->3V3, GND->GND, TXD->D5 (GPIO6), RXD->D4 (GPIO5), VT->3V3
//   Note: D4/D5 are silkscreen labels on Seeed XIAO ESP32-S3 (actual GPIOs: D4=GPIO5, D5=GPIO6)
//
// IMPORTANT: Upload with correct Arduino CLI flags for Web Serial API compatibility:
//   --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default
//   USBMode=default   -> USB-OTG (TinyUSB) mode (required for Web Serial API)
//   CDCOnBoot=default -> USB CDC stays enabled after boot (required for browser connection)
//
// Config: USB-C cable, Serial @ 115200 baud

#include "sdkconfig.h"
#include <HardwareSerial.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "SerialCommandHandler.h"

#include <USB.h>
#include <USBHIDKeyboard.h>
#include <BleKeyboard.h>

#if CONFIG_IDF_TARGET_ESP32S3
  #define FP_TX_PIN 5   // D4 - Fingerprint sensor TX (GPIO5)
  #define FP_RX_PIN 6   // D5 - Fingerprint sensor RX (GPIO6)
  // Config serial uses UART0 (GPIO43/44) via Serial object when USB CDC On Boot is disabled
#else
  #define FP_TX_PIN 16
  #define FP_RX_PIN 17
#endif

#define FP_HEADER 0xEF01
#define FP_DEFAULT_ADDR 0xFFFFFFFF
#define FP_CMD_PACKET 0x01

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

#define LED_RED 0x01
#define LED_GREEN 0x04
#define LED_FLASHING 0x02
#define LED_ON 0x03
#define LED_OFF 0x04

HardwareSerial fpSerial(1);
SerialCommandHandler cmdHandler;

// Both keyboard types available
USBHIDKeyboard usbKeyboard;
BleKeyboard bleKeyboard("TouchPass", "Anthropic", 100);
bool usbInitialized = false;
bool bleInitialized = false;
bool useUsb = false;  // Default to BLE for ESP32-S3 (USB Serial for config only)

Preferences prefs;

uint32_t fpAddress = FP_DEFAULT_ADDR;
uint16_t templateCount = 0;
uint16_t librarySize = 200;
String lastStatus = "Ready";
bool sensorOk = false;

enum EnrollState {
    ENROLL_IDLE,
    ENROLL_CAPTURE_1, ENROLL_LIFT_1,
    ENROLL_CAPTURE_2, ENROLL_LIFT_2,
    ENROLL_CAPTURE_3, ENROLL_LIFT_3,
    ENROLL_CAPTURE_4, ENROLL_LIFT_4,
    ENROLL_CAPTURE_5, ENROLL_LIFT_5,
    ENROLL_CAPTURE_6,
    ENROLL_MERGING,
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

const char* enrollMessages[] = {
    "Place finger on sensor",
    "Lift and place again",
    "Again, adjust slightly",
    "Now adjust your grip",
    "Place again",
    "One more time"
};

String lastDetectedFinger = "";
int lastDetectedId = -1;
int lastDetectedScore = 0;
uint8_t lastDetectResult = 0xFF;
bool newDetectionAvailable = false;

uint8_t rxBuffer[256];

bool isKeyboardConnected() {
    if (useUsb) {
        return usbInitialized;
    } else {
        return bleKeyboard.isConnected();
    }
}

String getKeyboardMode() {
    return useUsb ? "USB-HID" : "BLE";
}

void typePassword(uint16_t fingerId) {
    String pwd = getFingerPassword(fingerId);
    if (pwd.length() == 0 || !isKeyboardConnected()) return;

    if (useUsb) {
        usbKeyboard.releaseAll();
        delay(50);
        for (unsigned int i = 0; i < pwd.length(); i++) {
            usbKeyboard.print(String(pwd[i]));
            delay(10);
        }
        if (getFingerPressEnter(fingerId)) {
            delay(50);
            usbKeyboard.write(KEY_RETURN);
        }
        usbKeyboard.releaseAll();
    } else {
        bleKeyboard.releaseAll();
        delay(50);
        for (unsigned int i = 0; i < pwd.length(); i++) {
            bleKeyboard.print(String(pwd[i]));
            delay(30);
        }
        if (getFingerPressEnter(fingerId)) {
            delay(50);
            bleKeyboard.write(KEY_RETURN);
        }
        bleKeyboard.releaseAll();
    }
}

uint16_t sendCommand(uint8_t cmd, uint8_t* data, uint16_t dataLen) {
    uint16_t len = dataLen + 3;
    uint16_t sum = FP_CMD_PACKET + (len >> 8) + (len & 0xFF) + cmd;

    fpSerial.write((FP_HEADER >> 8) & 0xFF);
    fpSerial.write(FP_HEADER & 0xFF);
    fpSerial.write((fpAddress >> 24) & 0xFF);
    fpSerial.write((fpAddress >> 16) & 0xFF);
    fpSerial.write((fpAddress >> 8) & 0xFF);
    fpSerial.write(fpAddress & 0xFF);
    fpSerial.write(FP_CMD_PACKET);
    fpSerial.write((len >> 8) & 0xFF);
    fpSerial.write(len & 0xFF);
    fpSerial.write(cmd);

    for (uint16_t i = 0; i < dataLen; i++) {
        fpSerial.write(data[i]);
        sum += data[i];
    }

    fpSerial.write((sum >> 8) & 0xFF);
    fpSerial.write(sum & 0xFF);
    return len + 9;
}

int16_t receiveResponse(uint8_t* buffer, uint16_t timeout) {
    unsigned long start = millis();
    uint16_t idx = 0;
    uint16_t packetLen = 0;
    bool headerFound = false;

    while (millis() - start < timeout) {
        if (fpSerial.available()) {
            uint8_t b = fpSerial.read();
            buffer[idx++] = b;

            if (idx == 2) {
                if (buffer[0] == 0xEF && buffer[1] == 0x01) {
                    headerFound = true;
                } else {
                    buffer[0] = buffer[1];
                    idx = 1;
                }
            }

            if (headerFound && idx == 9) {
                packetLen = (buffer[7] << 8) | buffer[8];
            }

            if (headerFound && idx >= 9 && idx >= 9 + packetLen) {
                return idx;
            }

            if (idx >= sizeof(rxBuffer) - 1) return -1;
        }
    }
    return headerFound ? idx : -1;
}

uint8_t getConfirmCode(uint8_t* buffer, int16_t len) {
    if (len < 10) return 0xFF;
    return buffer[9];
}

bool checkSensorConnection() {
    sendCommand(CMD_HANDSHAKE, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 500);
    if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
        sensorOk = true;
        return true;
    }
    sendCommand(CMD_CHECKSENSOR, NULL, 0);
    len = receiveResponse(rxBuffer, 500);
    sensorOk = (len > 0 && getConfirmCode(rxBuffer, len) == 0x00);
    return sensorOk;
}

uint16_t getTemplateCount() {
    sendCommand(CMD_TEMPLATENUM, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 500);
    if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
        templateCount = (rxBuffer[10] << 8) | rxBuffer[11];
    }
    return templateCount;
}

bool readSysParams() {
    sendCommand(CMD_READSYSPARA, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 500);
    if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
        librarySize = (rxBuffer[14] << 8) | rxBuffer[15];
        return true;
    }
    return false;
}

bool setLED(uint8_t mode, uint8_t speed, uint8_t color, uint8_t count) {
    uint8_t data[4] = {mode, speed, color, count};
    sendCommand(CMD_AURALEDCONFIG, data, 4);
    int16_t len = receiveResponse(rxBuffer, 500);
    return (len > 0 && getConfirmCode(rxBuffer, len) == 0x00);
}

uint8_t captureImage() {
    sendCommand(CMD_GENIMG, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 3000);
    return (len > 0) ? getConfirmCode(rxBuffer, len) : 0xFF;
}

uint8_t generateChar(uint8_t bufferId) {
    uint8_t data[1] = {bufferId};
    sendCommand(CMD_IMG2TZ, data, 1);
    int16_t len = receiveResponse(rxBuffer, 2000);
    return (len > 0) ? getConfirmCode(rxBuffer, len) : 0xFF;
}

uint8_t createTemplate() {
    sendCommand(CMD_REGMODEL, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 2000);
    return (len > 0) ? getConfirmCode(rxBuffer, len) : 0xFF;
}

uint8_t storeTemplate(uint8_t bufferId, uint16_t id) {
    uint8_t data[3] = {bufferId, (uint8_t)(id >> 8), (uint8_t)(id & 0xFF)};
    sendCommand(CMD_STORE, data, 3);
    int16_t len = receiveResponse(rxBuffer, 2000);
    return (len > 0) ? getConfirmCode(rxBuffer, len) : 0xFF;
}

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

uint8_t deleteTemplate(uint16_t id, uint16_t count) {
    uint8_t data[4] = {(uint8_t)(id >> 8), (uint8_t)(id & 0xFF),
                       (uint8_t)(count >> 8), (uint8_t)(count & 0xFF)};
    sendCommand(CMD_DELETCHAR, data, 4);
    int16_t len = receiveResponse(rxBuffer, 2000);
    return (len > 0) ? getConfirmCode(rxBuffer, len) : 0xFF;
}

uint8_t emptyLibrary() {
    sendCommand(CMD_EMPTY, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 3000);
    return (len > 0) ? getConfirmCode(rxBuffer, len) : 0xFF;
}

int16_t findEmptySlot() {
    for (int page = 0; page < 1; page++) {
        uint8_t data[1] = {(uint8_t)page};
        sendCommand(CMD_READINDEXTABLE, data, 1);
        int16_t len = receiveResponse(rxBuffer, 1000);
        if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
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
    return -1;
}

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
    return -1;
}

void saveFingerName(uint16_t id, String name) {
    prefs.begin("fingers", false);
    prefs.putString(("f" + String(id)).c_str(), name);
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
    prefs.remove(("i" + String(id)).c_str());
    prefs.end();
}

void clearAllFingerNames() {
    prefs.begin("fingers", false);
    prefs.clear();
    prefs.end();
}

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

int getFingerIdForSlot(uint16_t slot) {
    prefs.begin("fingers", true);
    int fingerId = prefs.getInt(("i" + String(slot)).c_str(), -1);
    prefs.end();
    return fingerId;
}

void processFingerDetection() {
    if (enrollState != ENROLL_IDLE) return;

    static unsigned long lastAttempt = 0;
    if (millis() - lastAttempt < 500) return;
    lastAttempt = millis();

    uint8_t result = captureImage();
    if (result != 0x00) return;

    result = generateChar(1);
    if (result != 0x00) {
        lastDetectResult = result;
        newDetectionAvailable = true;
        lastStatus = "Unknown finger";
        setLED(LED_ON, 0, LED_RED, 0);
        delay(2000);
        setLED(LED_OFF, 0, LED_RED, 0);
        while (captureImage() == 0x00) delay(200);
        return;
    }

    uint16_t matchId = 0, score = 0;
    result = searchFingerprint(1, 0, librarySize, &matchId, &score);

    if (result == 0x00) {
        lastDetectedFinger = getFingerName(matchId);
        lastDetectedId = matchId;
        lastDetectedScore = score;
        lastDetectResult = 0x00;
        newDetectionAvailable = true;
        lastStatus = lastDetectedFinger + " detected";
        setLED(LED_ON, 0, LED_GREEN, 0);

        typePassword(matchId);

        delay(1000);
        setLED(LED_OFF, 0, LED_GREEN, 0);
        while (captureImage() == 0x00) delay(200);

    } else {
        lastDetectedFinger = "";
        lastDetectedId = -1;
        lastDetectedScore = 0;
        lastDetectResult = result;
        newDetectionAvailable = true;
        lastStatus = "Unknown finger";
        setLED(LED_ON, 0, LED_RED, 0);
        delay(2000);
        setLED(LED_OFF, 0, LED_RED, 0);
        while (captureImage() == 0x00) delay(200);
    }
}

bool enrollCaptureToBuffer(uint8_t bufferNum) {
    uint8_t result = captureImage();
    if (result != 0x00) return false;

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

bool isFingerLifted() {
    return captureImage() == 0x02;
}

void processEnrollment() {
    if (enrollState == ENROLL_IDLE) return;

    if (millis() > enrollTimeout) {
        enrollState = ENROLL_DONE;
        enrollSuccess = false;
        enrollError = "Timeout";
        setLED(LED_ON, 0, LED_RED, 0);
        return;
    }

    uint8_t result;

    switch (enrollState) {
        case ENROLL_CAPTURE_1:
            if (enrollCaptureToBuffer(1)) {
                enrollState = ENROLL_LIFT_1;
                setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_1:
            if (isFingerLifted()) {
                enrollState = ENROLL_CAPTURE_2;
                setLED(LED_OFF, 0, LED_GREEN, 0);
            }
            break;

        case ENROLL_CAPTURE_2:
            if (enrollCaptureToBuffer(2)) {
                enrollState = ENROLL_LIFT_2;
                setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_2:
            if (isFingerLifted()) {
                enrollState = ENROLL_CAPTURE_3;
                setLED(LED_OFF, 0, LED_GREEN, 0);
            }
            break;

        case ENROLL_CAPTURE_3:
            if (enrollCaptureToBuffer(3)) {
                enrollState = ENROLL_LIFT_3;
                setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_3:
            if (isFingerLifted()) {
                enrollState = ENROLL_CAPTURE_4;
                setLED(LED_OFF, 0, LED_GREEN, 0);
            }
            break;

        case ENROLL_CAPTURE_4:
            if (enrollCaptureToBuffer(4)) {
                enrollState = ENROLL_LIFT_4;
                setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_4:
            if (isFingerLifted()) {
                enrollState = ENROLL_CAPTURE_5;
                setLED(LED_OFF, 0, LED_GREEN, 0);
            }
            break;

        case ENROLL_CAPTURE_5:
            if (enrollCaptureToBuffer(5)) {
                enrollState = ENROLL_LIFT_5;
                setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_5:
            if (isFingerLifted()) {
                enrollState = ENROLL_CAPTURE_6;
                setLED(LED_OFF, 0, LED_GREEN, 0);
            }
            break;

        case ENROLL_CAPTURE_6:
            if (enrollCaptureToBuffer(6)) {
                enrollState = ENROLL_MERGING;
                setLED(LED_OFF, 0, LED_GREEN, 0);
            }
            break;

        case ENROLL_MERGING:
            result = createTemplate();
            if (result != 0x00) {
                enrollError = "Template merge failed";
                enrollState = ENROLL_DONE;
                enrollSuccess = false;
                setLED(LED_ON, 0, LED_RED, 0);
                return;
            }

            result = storeTemplate(1, pendingSlot);
            if (result != 0x00) {
                enrollError = "Store failed";
                enrollState = ENROLL_DONE;
                enrollSuccess = false;
                setLED(LED_ON, 0, LED_RED, 0);
                return;
            }

            saveFingerName(pendingSlot, pendingFingerName);
            if (pendingFingerPassword.length() > 0) {
                saveFingerPassword(pendingSlot, pendingFingerPassword);
                saveFingerPressEnter(pendingSlot, pendingPressEnter);
            }
            getTemplateCount();
            enrollState = ENROLL_DONE;
            enrollSuccess = true;
            delay(50);
            setLED(LED_ON, 0, LED_GREEN, 0);
            delay(500);
            setLED(LED_OFF, 0, LED_GREEN, 0);
            lastStatus = pendingFingerName + " enrolled";
            pendingFingerPassword = "";
            break;

        default:
            break;
    }
}

// ===== Serial Command Handler Functions =====

String getStatusJson() {
    getTemplateCount();
    String json = "{\"sensor\":" + String(sensorOk ? "true" : "false") +
                  ",\"count\":" + String(templateCount) +
                  ",\"capacity\":" + String(librarySize) +
                  ",\"last\":\"" + lastStatus + "\"}";
    return json;
}

String getDetectJson() {
    String json = "{\"detected\":" + String(newDetectionAvailable ? "true" : "false") +
                  ",\"finger\":\"" + lastDetectedFinger + "\"" +
                  ",\"id\":" + String(lastDetectedId) +
                  ",\"score\":" + String(lastDetectedScore) +
                  ",\"matched\":" + String(lastDetectedId >= 0 ? "true" : "false") +
                  ",\"lastResult\":\"0x" + String(lastDetectResult, HEX) + "\"}";
    if (newDetectionAvailable) newDetectionAvailable = false;
    return json;
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
                            json += "{\"id\":" + String(id) + ",\"name\":\"" + getFingerName(id) + "\",\"fingerId\":" + String(getFingerIdForSlot(id)) + "}";
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
    if (!params.containsKey("name")) {
        return "{\"ok\":false,\"status\":\"Missing name\"}";
    }

    pendingFingerName = params["name"].as<String>();
    pendingFingerPassword = params.containsKey("password") ? params["password"].as<String>() : "";
    pendingPressEnter = params.containsKey("pressEnter") && params["pressEnter"].as<bool>();
    pendingFingerId = params.containsKey("finger") ? params["finger"].as<int>() : -1;

    int16_t existingSlot = findSlotForFinger(pendingFingerId);
    if (existingSlot >= 0) {
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
    enrollTimeout = millis() + 60000;
    lastStatus = "Enrolling " + pendingFingerName;

    return "{\"ok\":true,\"status\":\"Place finger on sensor\"}";
}

String getEnrollStatusJson() {
    String json = "{\"state\":" + String(enrollState) + ",\"totalSteps\":6,";

    int step = 0;
    bool captured = false;
    bool done = false;
    const char* phase = "center";
    const char* message = "";

    switch (enrollState) {
        case ENROLL_IDLE: step = 0; break;
        case ENROLL_CAPTURE_1: step = 1; message = enrollMessages[0]; break;
        case ENROLL_LIFT_1: step = 1; captured = true; message = enrollMessages[1]; break;
        case ENROLL_CAPTURE_2: step = 2; message = enrollMessages[1]; break;
        case ENROLL_LIFT_2: step = 2; captured = true; message = enrollMessages[2]; break;
        case ENROLL_CAPTURE_3: step = 3; message = enrollMessages[2]; break;
        case ENROLL_LIFT_3: step = 3; captured = true; phase = "edges"; message = enrollMessages[3]; break;
        case ENROLL_CAPTURE_4: step = 4; phase = "edges"; message = enrollMessages[3]; break;
        case ENROLL_LIFT_4: step = 4; captured = true; phase = "edges"; message = enrollMessages[4]; break;
        case ENROLL_CAPTURE_5: step = 5; phase = "edges"; message = enrollMessages[4]; break;
        case ENROLL_LIFT_5: step = 5; captured = true; phase = "edges"; message = enrollMessages[5]; break;
        case ENROLL_CAPTURE_6: step = 6; phase = "edges"; message = enrollMessages[5]; break;
        case ENROLL_MERGING: step = 6; captured = true; phase = "edges"; message = "Creating template..."; break;
        case ENROLL_DONE: step = 6; captured = true; done = true; phase = "edges"; break;
    }

    json += "\"step\":" + String(step) +
            ",\"captured\":" + String(captured ? "true" : "false") +
            ",\"phase\":\"" + String(phase) + "\"" +
            ",\"message\":\"" + String(message) + "\"" +
            ",\"done\":" + String(done ? "true" : "false");

    if (enrollState == ENROLL_DONE) {
        json += ",\"ok\":" + String(enrollSuccess ? "true" : "false") +
                ",\"name\":\"" + pendingFingerName + "\"" +
                ",\"status\":\"" + (enrollSuccess ? String("Enrolled successfully") : enrollError) + "\"";
        enrollState = ENROLL_IDLE;
        if (!enrollSuccess) setLED(LED_ON, 0, LED_RED, 0);
        delay(100);
        setLED(LED_OFF, 0, LED_RED, 0);
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
    setLED(LED_OFF, 0, LED_GREEN, 0);
    return "{\"ok\":true}";
}

String deleteFingerJson(JsonObject params) {
    if (!params.containsKey("id")) {
        return "{\"ok\":false,\"status\":\"Missing ID\"}";
    }

    int id = params["id"].as<int>();
    if (id < 0 || id >= librarySize) {
        return "{\"ok\":false,\"status\":\"Invalid ID\"}";
    }

    String name = getFingerName(id);
    uint8_t result = deleteTemplate(id, 1);
    getTemplateCount();

    if (result == 0x00) {
        deleteFingerName(id);
        setLED(LED_ON, 0, LED_GREEN, 0);
        delay(500);
        setLED(LED_OFF, 0, LED_GREEN, 0);
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
        setLED(LED_OFF, 0, LED_GREEN, 0);
        lastStatus = "Library cleared";
        return "{\"ok\":true,\"status\":\"All fingerprints deleted\",\"count\":" + String(templateCount) + "}";
    } else {
        lastStatus = "Clear failed";
        return "{\"ok\":false,\"status\":\"Failed to clear library\",\"count\":" + String(templateCount) + "}";
    }
}

String getBLEStatusJson() {
    String json = "{\"connected\":" + String(isKeyboardConnected() ? "true" : "false") +
                  ",\"mode\":\"" + getKeyboardMode() + "\"}";
    return json;
}

String getFingerJson(JsonObject params) {
    if (!params.containsKey("id")) {
        return "{\"ok\":false,\"status\":\"Missing ID\"}";
    }

    int id = params["id"].as<int>();
    if (id < 0 || id >= librarySize) {
        return "{\"ok\":false,\"status\":\"Invalid ID\"}";
    }

    String json = "{\"ok\":true,\"id\":" + String(id) +
                  ",\"name\":\"" + getFingerName(id) + "\"" +
                  ",\"hasPassword\":" + String(getFingerPassword(id).length() > 0 ? "true" : "false") +
                  ",\"pressEnter\":" + String(getFingerPressEnter(id) ? "true" : "false") +
                  ",\"fingerId\":" + String(getFingerIdForSlot(id)) + "}";
    return json;
}

String updateFingerJson(JsonObject params) {
    if (!params.containsKey("id")) {
        return "{\"ok\":false,\"status\":\"Missing ID\"}";
    }

    int id = params["id"].as<int>();
    if (id < 0 || id >= librarySize) {
        return "{\"ok\":false,\"status\":\"Invalid ID\"}";
    }

    if (params.containsKey("name")) saveFingerName(id, params["name"].as<String>());
    if (params.containsKey("password")) saveFingerPassword(id, params["password"].as<String>());
    if (params.containsKey("pressEnter")) saveFingerPressEnter(id, params["pressEnter"].as<bool>());

    return "{\"ok\":true,\"status\":\"Updated " + getFingerName(id) + "\"}";
}

String getSystemInfoJson() {
    String json = "{\"chip\":\"";
    #if CONFIG_IDF_TARGET_ESP32S3
        json += "ESP32-S3";
    #else
        json += "ESP32-C6";
    #endif
    json += "\",\"firmware\":\"TouchPass v1.0\"";
    json += ",\"freeHeap\":" + String(ESP.getFreeHeap());
    json += "}";
    return json;
}

String getKeyboardModeJson() {
    String json = "{";

    // Current active mode
    json += "\"current\":\"" + getKeyboardMode() + "\"";

    // Available modes based on chip
    json += ",\"availableModes\":[";
    #if CONFIG_IDF_TARGET_ESP32S3
        json += "\"BLE\",\"USB-HID\"";
    #else
        json += "\"BLE\"";
    #endif
    json += "]";

    // Saved preference
    String savedMode = useUsb ? "USB-HID" : "BLE";
    json += ",\"saved\":\"" + savedMode + "\"";

    json += "}";
    return json;
}

String setKeyboardModeJson(JsonObject params) {
    if (params.containsKey("mode")) {
        String mode = params["mode"].as<String>();
        bool newUseUsb = (mode == "usb");
        if (newUseUsb != useUsb) {
            useUsb = newUseUsb;
            prefs.begin("settings", false);
            prefs.putBool("useUsb", useUsb);
            prefs.end();
            String json = "{\"ok\":true,\"mode\":\"" + getKeyboardMode() + "\",\"restart\":true}";
            delay(500);
            ESP.restart();
            return json;
        }
    }
    return "{\"ok\":true,\"mode\":\"" + getKeyboardMode() + "\"}";
}

String rebootJson() {
    delay(500);
    ESP.restart();
    return "{\"ok\":true,\"status\":\"Rebooting\"}";
}

String getDiagnosticsJson() {
    String json = "{";

    // UART1 info
    json += "\"uart1\":{";
    json += "\"rxPin\":" + String(FP_RX_PIN);
    json += ",\"txPin\":" + String(FP_TX_PIN);
    json += ",\"baud\":57600";
    json += ",\"available\":" + String(fpSerial.available());
    json += "}";

    // USB info
    json += ",\"usb\":{";
    json += "\"hid\":" + String(usbInitialized ? "true" : "false");
    json += ",\"serial\":" + String(Serial ? "true" : "false");
    json += ",\"mode\":\"" + String(useUsb ? "usb" : "ble") + "\"";
    json += "}";

    // Sensor info
    json += ",\"sensor\":{";
    json += "\"connected\":" + String(sensorOk ? "true" : "false");
    json += ",\"library\":\"" + lastStatus + "\"";
    json += "}";

    // Chip info
    json += ",\"chip\":{";
    json += "\"model\":\"" + String(ESP.getChipModel()) + "\"";
    json += ",\"cores\":" + String(ESP.getChipCores());
    json += ",\"freeHeap\":" + String(ESP.getFreeHeap());
    json += "}";

    json += "}";
    return json;
}

void setup() {
    // Load keyboard mode preference (default to BLE for ESP32-S3)
    prefs.begin("settings", true);
    useUsb = prefs.getBool("useUsb", false);
    prefs.end();

    // Initialize USB subsystem (required for both USB HID and Serial CDC)
    USB.begin();
    delay(100);

    // Initialize keyboard based on preference
    if (useUsb) {
        usbKeyboard.begin();
        delay(500);
        usbInitialized = true;
    } else {
        bleKeyboard.begin();
        bleInitialized = true;
    }

    // Initialize USB Serial for configuration (native USB CDC)
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) delay(10);

    cmdHandler.begin(&Serial);

    fpSerial.begin(57600, SERIAL_8N1, FP_RX_PIN, FP_TX_PIN);
    delay(500);

    // Clear any garbage in buffer
    while (fpSerial.available()) {
        fpSerial.read();
    }

    if (checkSensorConnection()) {
        readSysParams();
        getTemplateCount();
    } else {
        setLED(LED_ON, 0, LED_RED, 0);
    }
}

void loop() {
    cmdHandler.loop();
    processEnrollment();
    processFingerDetection();
}
