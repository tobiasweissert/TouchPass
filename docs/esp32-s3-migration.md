# ESP32-S3 Migration Notes

This document captures learnings from migrating TouchPass from ESP32-C6 to ESP32-S3.

## Summary of Changes

### Platform Migration

| Aspect | ESP32-C6 | ESP32-S3 |
|--------|----------|----------|
| Keyboard | BLE only | USB HID + BLE |
| Serial pins (D6/D7) | GPIO16/17 | GPIO43/44 |
| WiFi antenna | Built-in | External required |
| Configuration | WiFi AP | WiFi AP |

### Configuration Method Change

Originally tried USB Serial (Web Serial API) for configuration on S3, but reverted to WiFi AP:
- Web Serial worked inconsistently on S3
- WiFi AP is more reliable and works from any browser
- WiFi starts enabled by default for initial setup

## Key Learnings

### 1. XIAO ESP32-S3 Pin Mapping

The XIAO board silkscreen labels (D0-D10) map to different GPIOs than ESP32-C6:

| Label | ESP32-S3 GPIO | ESP32-C6 GPIO |
|-------|---------------|---------------|
| D6 | GPIO43 | GPIO16 |
| D7 | GPIO44 | GPIO17 |
| D4 | GPIO5 | GPIO4 |
| D2 | GPIO3 | GPIO2 |

**Solution**: Use conditional compilation:
```cpp
#if CONFIG_IDF_TARGET_ESP32S3
  #define FP_TX_PIN 5   // D4 - matches Seeed XIAO ESP32-S3 pinout
  #define FP_RX_PIN 6   // D5 - matches Seeed XIAO ESP32-S3 pinout
#else
  #define FP_TX_PIN 16  // D6 - ESP32-C6
  #define FP_RX_PIN 17  // D7 - ESP32-C6
#endif
```

### 2. ESP32-S3 WiFi Requires External Antenna

The XIAO ESP32-S3 has minimal built-in antenna. WiFi won't work without the external antenna connected to the U.FL connector.

**Symptoms**: Device boots, sensor works, but no WiFi network visible.

**Solution**: Connect the included external antenna to the U.FL connector on the bottom-left of the board.

### 3. WiFi AP Initialization on S3

WiFi AP mode requires specific initialization sequence on S3:

```cpp
void startWifi() {
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_AP);
    delay(100);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.softAP(ssid, password, 1, false, 4);
    delay(1000);
    server.begin();
}
```

Key points:
- Reset WiFi mode before starting AP
- Set TX power to maximum (19.5 dBm)
- Add delays for stability
- Use channel 1 for best compatibility

### 4. USB HID + BLE Library Conflict

The USBHIDKeyboard and ESP32-BLE-Keyboard libraries both define:
- `KEY_LEFT_CTRL`, `KEY_RETURN`, etc. (macros vs const)
- `KeyReport` typedef

**Solution**: Patch both libraries with preprocessor guards. See [library-patches.md](library-patches.md).

### 5. LED Commands Need Delay

The R502-A fingerprint sensor LED commands sometimes fail on first call:

```cpp
void setIdleLED() {
    delay(50);  // Required for reliable LED control
    setLED(LED_BREATHING, 100, LED_BLUE, 0);
}
```

### 6. USB Port Disappears During Development

When flashing unstable firmware, the USB CDC port can disappear, making the device unflashable.

**Recovery**:
1. Unplug USB
2. Hold BOOT button on XIAO board
3. Plug in USB while holding BOOT
4. Release BOOT after 1 second
5. Flash new firmware

### 7. GPIO Cannot Power 30mA Sensor

Attempted to power the R502-A sensor directly from GPIO (for wake-on-touch feature) using GPIO_DRIVE_CAP_3 (40mA spec).

**Result**: Sensor did not work reliably with GPIO power.

**Solution**: Use 3V3 pin for sensor VCC. Wake-on-touch feature abandoned.

### 8. Keyboard Mode Stored in Preferences

Keyboard mode (USB vs BLE) is stored in non-volatile storage:

```cpp
// Load on startup
prefs.begin("settings", true);
useUsb = prefs.getBool("useUsb", true);
prefs.end();

// Save when changed
prefs.begin("settings", false);
prefs.putBool("useUsb", useUsb);
prefs.end();
ESP.restart();  // Restart required to reinitialize keyboard
```

### 9. Both Keyboards Cannot Run Simultaneously

Even with library patches, only one keyboard can be active at a time. Switching requires device restart.

## Troubleshooting: No Serial Output

If you see no serial output when connecting USB:

1. **Check USB CDC On Boot setting**: Tools → USB CDC On Boot → **Disabled**
2. **Try different USB cable**: Some cables are power-only
3. **Check USB port**: Try different USB port on computer
4. **Verify board selection**: Tools → Board → XIAO_ESP32S3
5. **Check serial monitor baud rate**: Must be 115200

If still no output, the bootloader may not be responding. Try:
- Hold BOOT button, click RESET, release BOOT to enter download mode
- Re-upload firmware with verbose output enabled

## Firmware Architecture

```
firmware.ino
├── Includes (USB, BLE, WiFi, WebServer)
├── Pin definitions (conditional)
├── Global state
├── Keyboard functions
│   ├── isKeyboardConnected()
│   ├── getKeyboardMode()
│   └── typePassword()
├── Sensor communication
│   ├── sendCommand() / receiveResponse()
│   └── Fingerprint operations
├── Preferences storage
├── LED control
├── WiFi management
├── Web handlers
├── Enrollment state machine
└── Main loop
```

## Compile Commands

**ESP32-S3:**
```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3:PartitionScheme=huge_app firmware/
arduino-cli upload -p /dev/cu.usbmodem* --fqbn esp32:esp32:esp32s3:PartitionScheme=huge_app firmware/
```

**ESP32-C6:**
```bash
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C6:PartitionScheme=huge_app firmware/
arduino-cli upload -p /dev/cu.usbmodem* --fqbn esp32:esp32:XIAO_ESP32C6:PartitionScheme=huge_app firmware/
```

## API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Web UI |
| `/status` | GET | Sensor status, finger count |
| `/detect` | GET | Last detection result |
| `/fingers` | GET | List enrolled fingers |
| `/enroll/start` | GET | Start enrollment |
| `/enroll/status` | GET | Enrollment progress |
| `/enroll/cancel` | GET | Cancel enrollment |
| `/delete?id=N` | GET | Delete finger |
| `/empty` | GET | Delete all fingers |
| `/kb/status` | GET | Keyboard status |
| `/kb/mode?mode=usb|ble` | GET | Switch keyboard mode |
| `/finger/get?id=N` | GET | Get finger details |
| `/finger/update` | GET | Update finger settings |

## Memory Usage (ESP32-S3)

```
Sketch: 1,219,398 bytes (38% of 3,145,728)
Global variables: 71,168 bytes (21% of 327,680)
```
