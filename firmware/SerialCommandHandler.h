#ifndef SERIAL_COMMAND_HANDLER_H
#define SERIAL_COMMAND_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Forward declarations of command handler functions (implemented in firmware.ino)
String getStatusJson();
String getDetectJson();
String getFingersJson();
String enrollStartJson(JsonObject params);
String getEnrollStatusJson();
String enrollCancelJson();
String deleteFingerJson(JsonObject params);
String emptyLibraryJson();
String getBLEStatusJson();
String getFingerJson(JsonObject params);
String updateFingerJson(JsonObject params);
String getSystemInfoJson();
String getKeyboardModeJson();
String setKeyboardModeJson(JsonObject params);
String rebootJson();
String getDiagnosticsJson();

class SerialCommandHandler {
private:
    String rxBuffer;
    Stream* serial;
    static const size_t MAX_BUFFER = 2048;
    static const size_t JSON_DOC_SIZE = 2048;

    void processLine(const String& line) {
        // Parse JSON command
        StaticJsonDocument<JSON_DOC_SIZE> commandDoc;
        DeserializationError error = deserializeJson(commandDoc, line);

        if (error) {
            sendError("Invalid JSON", -1);
            return;
        }

        // Extract command fields
        const char* cmd = commandDoc["cmd"];
        int id = commandDoc["id"] | -1;
        JsonObject params = commandDoc["params"];

        if (!cmd) {
            sendError("Missing cmd field", id);
            return;
        }

        // Execute command
        executeCommand(cmd, params, id);
    }

    void executeCommand(const char* cmd, JsonObject params, int id) {
        String dataJson = "";

        // Route command to appropriate handler
        if (strcmp(cmd, "get_status") == 0) {
            dataJson = getStatusJson();
        } else if (strcmp(cmd, "get_detect") == 0) {
            dataJson = getDetectJson();
        } else if (strcmp(cmd, "get_fingers") == 0) {
            dataJson = getFingersJson();
        } else if (strcmp(cmd, "enroll_start") == 0) {
            dataJson = enrollStartJson(params);
        } else if (strcmp(cmd, "enroll_status") == 0) {
            dataJson = getEnrollStatusJson();
        } else if (strcmp(cmd, "enroll_cancel") == 0) {
            dataJson = enrollCancelJson();
        } else if (strcmp(cmd, "delete_finger") == 0) {
            dataJson = deleteFingerJson(params);
        } else if (strcmp(cmd, "empty_library") == 0) {
            dataJson = emptyLibraryJson();
        } else if (strcmp(cmd, "get_ble_status") == 0) {
            dataJson = getBLEStatusJson();
        } else if (strcmp(cmd, "get_finger") == 0) {
            dataJson = getFingerJson(params);
        } else if (strcmp(cmd, "update_finger") == 0) {
            dataJson = updateFingerJson(params);
        } else if (strcmp(cmd, "get_system_info") == 0) {
            dataJson = getSystemInfoJson();
        } else if (strcmp(cmd, "get_keyboard_mode") == 0) {
            dataJson = getKeyboardModeJson();
        } else if (strcmp(cmd, "set_keyboard_mode") == 0) {
            dataJson = setKeyboardModeJson(params);
        } else if (strcmp(cmd, "reboot") == 0) {
            dataJson = rebootJson();
        } else if (strcmp(cmd, "diagnostics") == 0) {
            dataJson = getDiagnosticsJson();
        } else {
            sendError("Unknown command", id);
            return;
        }

        // Send response with data
        sendResponse("ok", dataJson, id);
    }

    void sendResponse(const char* status, const String& dataJson, int id) {
        serial->print("{\"status\":\"");
        serial->print(status);
        serial->print("\",\"data\":");
        serial->print(dataJson);
        if (id >= 0) {
            serial->print(",\"id\":");
            serial->print(id);
        }
        serial->println("}");
    }

    void sendError(const char* message, int id) {
        // Build error response: {"status":"error","message":"...","id":123}
        serial->print("{\"status\":\"error\",\"message\":\"");
        serial->print(message);
        serial->print("\"");
        if (id >= 0) {
            serial->print(",\"id\":");
            serial->print(id);
        }
        serial->println("}");
    }

public:
    SerialCommandHandler() : serial(nullptr) {}

    void begin(Stream* serialPort) {
        serial = serialPort;
        rxBuffer.reserve(MAX_BUFFER);
        rxBuffer = "";
    }

    void loop() {
        if (!serial) return;

        while (serial->available() > 0) {
            char c = serial->read();

            if (rxBuffer.length() >= MAX_BUFFER) {
                rxBuffer = "";
                sendError("Buffer overflow", -1);
                continue;
            }

            rxBuffer += c;

            if (c == '\n') {
                String line = rxBuffer;
                rxBuffer = "";
                line.trim();
                if (line.length() > 0) {
                    processLine(line);
                }
            }
        }
    }
};

#endif // SERIAL_COMMAND_HANDLER_H
