// TouchPass Fingerprint Sensor Module
// R502-A fingerprint sensor communication

#ifndef TOUCHPASS_FINGERPRINT_H
#define TOUCHPASS_FINGERPRINT_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "config.h"

class FingerprintSensor {
public:
    FingerprintSensor();

    // Initialization
    void begin();
    bool isConnected();

    // System operations
    bool readSystemParams();
    uint16_t getTemplateCount();
    uint16_t getLibrarySize();
    int16_t findEmptySlot();

    // LED control
    bool setLED(uint8_t mode, uint8_t speed, uint8_t color, uint8_t count);
    void setIdleLED(bool wifiEnabled);

    // Image capture and processing
    uint8_t captureImage();
    uint8_t generateCharacteristics(uint8_t bufferId);
    bool isFingerLifted();

    // Template operations
    uint8_t createTemplate();
    uint8_t storeTemplate(uint8_t bufferId, uint16_t id);
    uint8_t deleteTemplate(uint16_t id, uint16_t count = 1);
    uint8_t emptyLibrary();

    // Search
    uint8_t searchFingerprint(uint8_t bufferId, uint16_t startId, uint16_t count,
                              uint16_t* matchId, uint16_t* score);

    // Index table reading
    bool readIndexTable(uint8_t page, uint8_t* buffer);

private:
    HardwareSerial serial;
    uint8_t rxBuffer[256];
    uint32_t address;
    uint16_t templateCount;
    uint16_t librarySize;

    // Low-level protocol
    uint16_t sendCommand(uint8_t cmd, uint8_t* data, uint16_t dataLen);
    int16_t receiveResponse(uint8_t* buffer, uint16_t timeout);
    uint8_t getConfirmCode(uint8_t* buffer, int16_t len);
};

#endif // TOUCHPASS_FINGERPRINT_H
