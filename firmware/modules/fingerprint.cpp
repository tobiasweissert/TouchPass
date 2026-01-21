// TouchPass Fingerprint Sensor Implementation

#include "fingerprint.h"

FingerprintSensor::FingerprintSensor()
    : serial(1),
      address(FP_DEFAULT_ADDR),
      templateCount(0),
      librarySize(200) {
}

void FingerprintSensor::begin() {
    serial.begin(FP_BAUD_RATE, SERIAL_8N1, FP_RX_PIN, FP_TX_PIN);
    delay(500);

    // Clear buffer
    while (serial.available()) {
        serial.read();
    }
}

bool FingerprintSensor::isConnected() {
    // Try handshake first
    sendCommand(CMD_HANDSHAKE, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 500);
    if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
        return true;
    }

    // Try checksensor as fallback
    sendCommand(CMD_CHECKSENSOR, NULL, 0);
    len = receiveResponse(rxBuffer, 500);
    return (len > 0 && getConfirmCode(rxBuffer, len) == 0x00);
}

bool FingerprintSensor::readSystemParams() {
    sendCommand(CMD_READSYSPARA, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 500);
    if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
        librarySize = (rxBuffer[14] << 8) | rxBuffer[15];
        return true;
    }
    return false;
}

uint16_t FingerprintSensor::getTemplateCount() {
    sendCommand(CMD_TEMPLATENUM, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 500);
    if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
        templateCount = (rxBuffer[10] << 8) | rxBuffer[11];
    }
    return templateCount;
}

uint16_t FingerprintSensor::getLibrarySize() {
    return librarySize;
}

int16_t FingerprintSensor::findEmptySlot() {
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

bool FingerprintSensor::setLED(uint8_t mode, uint8_t speed, uint8_t color, uint8_t count) {
    uint8_t data[4] = {mode, speed, color, count};
    sendCommand(CMD_AURALEDCONFIG, data, 4);
    int16_t len = receiveResponse(rxBuffer, 500);
    return (len > 0 && getConfirmCode(rxBuffer, len) == 0x00);
}

void FingerprintSensor::setIdleLED(bool wifiEnabled) {
    delay(50);
    if (wifiEnabled) {
        setLED(LED_BREATHING, 100, LED_BLUE, 0);
    } else {
        setLED(LED_OFF, 0, LED_BLUE, 0);
    }
}

uint8_t FingerprintSensor::captureImage() {
    sendCommand(CMD_GENIMG, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, SENSOR_TIMEOUT_MS);
    return (len > 0) ? getConfirmCode(rxBuffer, len) : 0xFF;
}

uint8_t FingerprintSensor::generateCharacteristics(uint8_t bufferId) {
    uint8_t data[1] = {bufferId};
    sendCommand(CMD_IMG2TZ, data, 1);
    int16_t len = receiveResponse(rxBuffer, 2000);
    return (len > 0) ? getConfirmCode(rxBuffer, len) : 0xFF;
}

bool FingerprintSensor::isFingerLifted() {
    return captureImage() == 0x02;
}

uint8_t FingerprintSensor::createTemplate() {
    sendCommand(CMD_REGMODEL, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 2000);
    return (len > 0) ? getConfirmCode(rxBuffer, len) : 0xFF;
}

uint8_t FingerprintSensor::storeTemplate(uint8_t bufferId, uint16_t id) {
    uint8_t data[3] = {bufferId, (uint8_t)(id >> 8), (uint8_t)(id & 0xFF)};
    sendCommand(CMD_STORE, data, 3);
    int16_t len = receiveResponse(rxBuffer, 2000);
    return (len > 0) ? getConfirmCode(rxBuffer, len) : 0xFF;
}

uint8_t FingerprintSensor::deleteTemplate(uint16_t id, uint16_t count) {
    uint8_t data[4] = {(uint8_t)(id >> 8), (uint8_t)(id & 0xFF),
                       (uint8_t)(count >> 8), (uint8_t)(count & 0xFF)};
    sendCommand(CMD_DELETCHAR, data, 4);
    int16_t len = receiveResponse(rxBuffer, 2000);
    return (len > 0) ? getConfirmCode(rxBuffer, len) : 0xFF;
}

uint8_t FingerprintSensor::emptyLibrary() {
    sendCommand(CMD_EMPTY, NULL, 0);
    int16_t len = receiveResponse(rxBuffer, 3000);
    return (len > 0) ? getConfirmCode(rxBuffer, len) : 0xFF;
}

uint8_t FingerprintSensor::searchFingerprint(uint8_t bufferId, uint16_t startId,
                                             uint16_t count, uint16_t* matchId, uint16_t* score) {
    uint8_t data[5] = {bufferId, (uint8_t)(startId >> 8), (uint8_t)(startId & 0xFF),
                       (uint8_t)(count >> 8), (uint8_t)(count & 0xFF)};
    sendCommand(CMD_SEARCH, data, 5);
    int16_t len = receiveResponse(rxBuffer, SENSOR_TIMEOUT_MS);
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

bool FingerprintSensor::readIndexTable(uint8_t page, uint8_t* buffer) {
    uint8_t data[1] = {page};
    sendCommand(CMD_READINDEXTABLE, data, 1);
    int16_t len = receiveResponse(rxBuffer, 1000);
    if (len > 0 && getConfirmCode(rxBuffer, len) == 0x00) {
        memcpy(buffer, rxBuffer + 10, 32);
        return true;
    }
    return false;
}

// ===== Private Methods =====

uint16_t FingerprintSensor::sendCommand(uint8_t cmd, uint8_t* data, uint16_t dataLen) {
    uint16_t len = dataLen + 3;
    uint16_t sum = FP_CMD_PACKET + (len >> 8) + (len & 0xFF) + cmd;

    serial.write((FP_HEADER >> 8) & 0xFF);
    serial.write(FP_HEADER & 0xFF);
    serial.write((address >> 24) & 0xFF);
    serial.write((address >> 16) & 0xFF);
    serial.write((address >> 8) & 0xFF);
    serial.write(address & 0xFF);
    serial.write(FP_CMD_PACKET);
    serial.write((len >> 8) & 0xFF);
    serial.write(len & 0xFF);
    serial.write(cmd);

    for (uint16_t i = 0; i < dataLen; i++) {
        serial.write(data[i]);
        sum += data[i];
    }

    serial.write((sum >> 8) & 0xFF);
    serial.write(sum & 0xFF);
    return len + 9;
}

int16_t FingerprintSensor::receiveResponse(uint8_t* buffer, uint16_t timeout) {
    unsigned long start = millis();
    uint16_t idx = 0;
    uint16_t packetLen = 0;
    bool headerFound = false;

    while (millis() - start < timeout) {
        if (serial.available()) {
            uint8_t b = serial.read();
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

uint8_t FingerprintSensor::getConfirmCode(uint8_t* buffer, int16_t len) {
    if (len < 10) return 0xFF;
    return buffer[9];
}
