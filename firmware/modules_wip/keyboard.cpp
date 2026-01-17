// TouchPass Keyboard Implementation

#include "keyboard.h"

TouchPassKeyboard::TouchPassKeyboard()
    : currentMode(MODE_BLE),
      bleKeyboard("TouchPass", "Anthropic", 100),
      usbInitialized(false),
      bleInitialized(false) {
}

void TouchPassKeyboard::begin(KeyboardMode mode) {
    currentMode = mode;

    if (mode == MODE_USB) {
        USB.begin();
        usbKeyboard.begin();
        delay(500);
        usbInitialized = true;
    } else {
        bleKeyboard.begin();
        bleInitialized = true;
    }
}

KeyboardMode TouchPassKeyboard::getMode() {
    return currentMode;
}

const char* TouchPassKeyboard::getModeString() {
    return (currentMode == MODE_USB) ? "USB" : "BLE";
}

bool TouchPassKeyboard::isConnected() {
    if (currentMode == MODE_USB) {
        return usbInitialized;
    } else {
        return bleKeyboard.isConnected();
    }
}

void TouchPassKeyboard::typePassword(const String& password, bool pressEnter) {
    if (!isConnected() || password.length() == 0) {
        return;
    }

    releaseAll();
    delay(50);

    if (currentMode == MODE_USB) {
        // USB HID typing
        for (unsigned int i = 0; i < password.length(); i++) {
            usbKeyboard.print(String(password[i]));
            delay(10);
        }
        if (pressEnter) {
            delay(50);
            usbKeyboard.write(KEY_RETURN);
        }
    } else {
        // BLE typing
        for (unsigned int i = 0; i < password.length(); i++) {
            bleKeyboard.print(String(password[i]));
            delay(30);
        }
        if (pressEnter) {
            delay(50);
            bleKeyboard.write(KEY_RETURN);
        }
    }

    releaseAll();
}

void TouchPassKeyboard::releaseAll() {
    if (currentMode == MODE_USB) {
        usbKeyboard.releaseAll();
    } else {
        bleKeyboard.releaseAll();
    }
}
