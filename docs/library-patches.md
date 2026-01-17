# Library Patches for USB + BLE Keyboard Coexistence

The ESP32-BLE-Keyboard library and the built-in USBHIDKeyboard library both define the same constants and types, causing compilation errors when both are included. These patches allow both libraries to coexist.

## Problem

Both libraries define:
- `KEY_LEFT_CTRL`, `KEY_LEFT_SHIFT`, etc. (key constants)
- `KeyReport` typedef (key report structure)

## Solution

Add preprocessor guards to prevent duplicate definitions.

## Patch 1: BleKeyboard.h

**File location**: `~/Documents/Arduino/libraries/ESP32_BLE_Keyboard/src/BleKeyboard.h`

### Change 1: Wrap KEY_* constants

Find this block (around line 15):
```cpp
const uint8_t KEY_LEFT_CTRL = 0x80;
const uint8_t KEY_LEFT_SHIFT = 0x81;
// ... more KEY_* definitions ...
const uint8_t KEY_F24 = 0xFB;
```

Wrap it with:
```cpp
#ifndef KEY_LEFT_CTRL
const uint8_t KEY_LEFT_CTRL = 0x80;
const uint8_t KEY_LEFT_SHIFT = 0x81;
// ... keep all KEY_* definitions ...
const uint8_t KEY_F24 = 0xFB;
#endif
```

### Change 2: Wrap KeyReport typedef

Find this block (around line 85):
```cpp
//  Low level key report: up to 6 keys and shift, ctrl etc at once
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;
```

Change to:
```cpp
// Only define KeyReport if not already defined by USBHIDKeyboard
#ifndef _KEYREPORT_DEFINED
#define _KEYREPORT_DEFINED
//  Low level key report: up to 6 keys and shift, ctrl etc at once
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;
#endif
```

## Patch 2: USBHIDKeyboard.h

**File location**: `~/Library/Arduino15/packages/esp32/hardware/esp32/3.x.x/libraries/USB/src/USBHIDKeyboard.h`

Find this block (around line 144):
```cpp
//  Low level key report: up to 6 keys and shift, ctrl etc at once
typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;
```

Change to:
```cpp
//  Low level key report: up to 6 keys and shift, ctrl etc at once
#ifndef _KEYREPORT_DEFINED
#define _KEYREPORT_DEFINED
typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;
#endif
```

## Include Order

In your sketch, include USBHIDKeyboard first, then BleKeyboard:

```cpp
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <BleKeyboard.h>
```

This ensures USBHIDKeyboard's KEY_* macros are defined first, and BleKeyboard will skip its duplicate definitions.

## Related Issue

See [ESP32-BLE-Keyboard PR #253](https://github.com/T-vK/ESP32-BLE-Keyboard/pull/253) for the upstream fix.

## Verification

After applying patches, this should compile without errors:

```cpp
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <BleKeyboard.h>

USBHIDKeyboard usbKeyboard;
BleKeyboard bleKeyboard("Test", "Test", 100);

void setup() {
    // Both can be instantiated
}

void loop() {}
```
