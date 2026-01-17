// TouchPass Storage Implementation

#include "storage.h"

TouchPassStorage::TouchPassStorage() {
}

KeyboardMode TouchPassStorage::loadKeyboardMode() {
    prefs.begin("settings", true);
    bool useUsb = prefs.getBool("useUsb", false);  // Default to BLE
    prefs.end();
    return useUsb ? MODE_USB : MODE_BLE;
}

void TouchPassStorage::saveKeyboardMode(KeyboardMode mode) {
    prefs.begin("settings", false);
    prefs.putBool("useUsb", mode == MODE_USB);
    prefs.end();
}

void TouchPassStorage::saveFinger(uint16_t slotId, const FingerData& data) {
    prefs.begin("fingers", false);

    // Save name
    if (data.name.length() > 0) {
        prefs.putString(makeKey("f", slotId).c_str(), data.name);
    }

    // Save password
    if (data.password.length() > 0) {
        prefs.putString(makeKey("p", slotId).c_str(), data.password);
    }

    // Save press enter flag
    prefs.putBool(makeKey("e", slotId).c_str(), data.pressEnter);

    // Save finger ID (0-9 for hand mapping)
    if (data.fingerId >= 0 && data.fingerId <= 9) {
        prefs.putInt(makeKey("i", slotId).c_str(), data.fingerId);
    }

    prefs.end();
}

FingerData TouchPassStorage::loadFinger(uint16_t slotId) {
    FingerData data;
    data.fingerId = -1;

    prefs.begin("fingers", true);

    data.name = prefs.getString(makeKey("f", slotId).c_str(), "");
    if (data.name.length() == 0) {
        data.name = "Finger " + String(slotId);
    }

    data.password = prefs.getString(makeKey("p", slotId).c_str(), "");
    data.pressEnter = prefs.getBool(makeKey("e", slotId).c_str(), false);
    data.fingerId = prefs.getInt(makeKey("i", slotId).c_str(), -1);

    prefs.end();

    return data;
}

void TouchPassStorage::deleteFinger(uint16_t slotId) {
    prefs.begin("fingers", false);

    prefs.remove(makeKey("f", slotId).c_str());
    prefs.remove(makeKey("p", slotId).c_str());
    prefs.remove(makeKey("e", slotId).c_str());
    prefs.remove(makeKey("i", slotId).c_str());

    prefs.end();
}

void TouchPassStorage::clearAllFingers() {
    prefs.begin("fingers", false);
    prefs.clear();
    prefs.end();
}

int16_t TouchPassStorage::findSlotForFinger(int fingerId, uint16_t maxSlots) {
    if (fingerId < 0 || fingerId > 9) {
        return -1;
    }

    prefs.begin("fingers", true);

    for (uint16_t slot = 0; slot < maxSlots; slot++) {
        String key = makeKey("i", slot);
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

bool TouchPassStorage::hasFingerData(uint16_t slotId) {
    prefs.begin("fingers", true);
    String key = makeKey("f", slotId);
    bool exists = prefs.isKey(key.c_str());
    prefs.end();
    return exists;
}

String TouchPassStorage::makeKey(const char* prefix, uint16_t id) {
    return String(prefix) + String(id);
}
