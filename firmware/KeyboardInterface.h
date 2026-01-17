#ifndef KEYBOARD_INTERFACE_H
#define KEYBOARD_INTERFACE_H

#include <Arduino.h>
#include "sdkconfig.h"

// Keyboard mode enumeration
enum KeyboardMode {
  KB_MODE_BLE = 0,
  KB_MODE_USB = 1,
  KB_MODE_AUTO = 2
};

// Abstract keyboard interface
class KeyboardInterface {
public:
  virtual ~KeyboardInterface() {}
  virtual bool begin() = 0;
  virtual bool isConnected() = 0;
  virtual void print(const String& text) = 0;
  virtual void write(uint8_t key) = 0;
  virtual void releaseAll() = 0;
  virtual String getModeName() = 0;
  virtual KeyboardMode getMode() = 0;
};

// ========================================
// BLE Keyboard Implementation
// ========================================
#if !defined(USB_HID_ONLY)
#include <BleKeyboard.h>

class BLEKeyboardWrapper : public KeyboardInterface {
private:
  BleKeyboard* bleKb;

public:
  BLEKeyboardWrapper(const char* name, const char* manufacturer, uint8_t batteryLevel) {
    bleKb = new BleKeyboard(name, manufacturer, batteryLevel);
  }

  ~BLEKeyboardWrapper() {
    delete bleKb;
  }

  bool begin() override {
    bleKb->begin();
    return true;
  }

  bool isConnected() override {
    return bleKb->isConnected();
  }

  void print(const String& text) override {
    if (isConnected()) {
      for (unsigned int i = 0; i < text.length(); i++) {
        bleKb->print(String(text[i]));
        delay(30);  // 30ms delay per character for BLE
      }
    }
  }

  void write(uint8_t key) override {
    if (isConnected()) {
      bleKb->write(key);
    }
  }

  void releaseAll() override {
    if (isConnected()) {
      bleKb->releaseAll();
    }
  }

  String getModeName() override {
    return "BLE";
  }

  KeyboardMode getMode() override {
    return KB_MODE_BLE;
  }
};
#endif // !USB_HID_ONLY

// ========================================
// USB HID Keyboard Implementation (ESP32-S3 only)
// ========================================
// Note: USB HID and BLE keyboard cannot coexist due to KeyReport conflict
// Define USB_HID_ONLY to use USB keyboard (excludes BLE)
#if defined(USB_HID_ONLY) && CONFIG_IDF_TARGET_ESP32S3

#include <USB.h>
#include <USBHIDKeyboard.h>

class USBKeyboardWrapper : public KeyboardInterface {
private:
  USBHIDKeyboard* usbKb;
  bool initialized;

public:
  USBKeyboardWrapper() : initialized(false) {
    usbKb = new USBHIDKeyboard();
  }

  ~USBKeyboardWrapper() {
    delete usbKb;
  }

  bool begin() override {
    if (!initialized) {
      usbKb->begin();
      USB.begin();
      initialized = true;
      delay(500);  // Allow host to recognize device
    }
    return true;
  }

  bool isConnected() override {
    // USB HID is always "connected" when plugged in
    return initialized && USB;
  }

  void print(const String& text) override {
    if (initialized) {
      for (unsigned int i = 0; i < text.length(); i++) {
        usbKb->print(String(text[i]));
        delay(10);  // 10ms delay per character for USB
      }
    }
  }

  void write(uint8_t key) override {
    if (initialized) {
      usbKb->write(key);
    }
  }

  void releaseAll() override {
    if (initialized) {
      usbKb->releaseAll();
    }
  }

  String getModeName() override {
    return "USB-HID";
  }

  KeyboardMode getMode() override {
    return KB_MODE_USB;
  }
};

#endif  // USB_HID_ONLY && CONFIG_IDF_TARGET_ESP32S3

#endif  // KEYBOARD_INTERFACE_H
