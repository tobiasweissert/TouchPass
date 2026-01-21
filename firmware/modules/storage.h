// TouchPass Storage Module
// Handles fingerprint data persistence

#ifndef TOUCHPASS_STORAGE_H
#define TOUCHPASS_STORAGE_H

#include <Arduino.h>
#include <Preferences.h>
#include "keyboard.h"

struct FingerData {
    String name;
    String password;
    bool pressEnter;
    int fingerId;  // 0-9 for hand mapping
};

class TouchPassStorage {
public:
    TouchPassStorage();

    // Keyboard mode settings
    KeyboardMode loadKeyboardMode();
    void saveKeyboardMode(KeyboardMode mode);

    // Finger data operations
    void saveFinger(uint16_t slotId, const FingerData& data);
    FingerData loadFinger(uint16_t slotId);
    void deleteFinger(uint16_t slotId);
    void clearAllFingers();

    // Find slot by finger ID
    int16_t findSlotForFinger(int fingerId, uint16_t maxSlots);

    // Check if finger has data
    bool hasFingerData(uint16_t slotId);

private:
    Preferences prefs;

    String makeKey(const char* prefix, uint16_t id);
};

#endif // TOUCHPASS_STORAGE_H
