// TouchPass Keyboard Module
// Abstracts USB HID and BLE keyboard

#ifndef TOUCHPASS_KEYBOARD_H
#define TOUCHPASS_KEYBOARD_H

#include <Arduino.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <BleKeyboard.h>

enum KeyboardMode {
    MODE_USB,
    MODE_BLE
};

class TouchPassKeyboard {
public:
    TouchPassKeyboard();

    // Initialization
    void begin(KeyboardMode mode = MODE_BLE);
    KeyboardMode getMode();
    const char* getModeString();

    // Status
    bool isConnected();

    // Typing
    void typePassword(const String& password, bool pressEnter = false);
    void releaseAll();

private:
    KeyboardMode currentMode;
    USBHIDKeyboard usbKeyboard;
    BleKeyboard bleKeyboard;
    bool usbInitialized;
    bool bleInitialized;
};

#endif // TOUCHPASS_KEYBOARD_H
