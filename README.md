# TouchPass

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A hardware password manager that types your passwords when you touch it with an enrolled finger.

## Features

- **6-capture enrollment** - Touch ID style enrollment for better accuracy
- **Dual Keyboard Support** - USB HID or BLE Keyboard (switchable via web UI)
- **WiFi AP Configuration** - Configure via web interface at 192.168.4.1
- **USB Serial Configuration** - Alternative USB-based config (no WiFi interference)
- **10 finger support** - Map passwords to specific fingers on left/right hands
- **Re-enrollment protection** - Re-enrolling a finger overwrites the previous entry
- **ESP32-S3 & ESP32-C6 support** - Works with both chips

## Hardware

| Component | Description |
|-----------|-------------|
| Seeed XIAO ESP32-S3 | Recommended - USB HID + WiFi + BLE |
| Seeed XIAO ESP32-C6 | Alternative - BLE only, WiFi 6 |
| R502-A | Capacitive fingerprint sensor with RGB LED |

See [hardware/README.md](hardware/README.md) for wiring diagram and details.

### Wiring

#### ESP32-S3 Fingerprint Sensor
| R502-A Pin | ESP32-S3 Pin | GPIO | Description |
|------------|--------------|------|-------------|
| VCC (Red)  | 3V3          | -    | Power |
| GND (Black)| GND          | -    | Ground |
| TXD (Yellow)| D5          | 6    | Sensor TX → ESP32 RX |
| RXD (Green)| D4           | 5    | Sensor RX ← ESP32 TX |
| VT (White) | 3V3          | -    | Touch power (always on) |

#### ESP32-C6 Fingerprint Sensor
| R502-A Pin | ESP32-C6 Pin | GPIO | Description |
|------------|--------------|------|-------------|
| VCC (Red)  | 3V3          | -    | Power |
| GND (Black)| GND          | -    | Ground |
| TXD (Yellow)| D7          | 17   | Sensor TX → ESP32 RX |
| RXD (Green)| D6           | 16   | Sensor RX ← ESP32 TX |
| VT (White) | 3V3          | -    | Touch power (always on) |

#### USB Serial Configuration (Alternative to WiFi)
For serial-based configuration without WiFi interference:
- **ESP32-S3**: Uses native USB CDC Serial @ 115200 baud (requires correct USB Mode settings)
- **ESP32-C6**: Uses native USB CDC Serial @ 115200 baud
- No additional wiring required - just USB-C cable
- Web Serial API interface in browser (Chrome/Edge)
- See [docs/serial-configuration.md](docs/serial-configuration.md) for details

## Quick Start

1. **Build** - See [docs/assembly.md](docs/assembly.md)
2. **Flash** - Upload firmware with correct settings:
   ```bash
   arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default firmware/firmware.ino
   arduino-cli upload -p /dev/cu.usbmodem1101 --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default firmware/firmware.ino
   ```
   **See [Programming Guide](docs/programming-guide.md) for details**
3. **Configure** - Open `firmware/config.html` in Chrome/Edge → Click "Connect Device"
4. **Enroll** - Tap a finger on the hand diagram, set name and password
5. **Use** - Touch enrolled finger to type password

## Project Structure

```
TouchPass/
├── firmware/          # ESP32 source code
│   ├── firmware.ino   # Main firmware
│   ├── webpage.h      # Embedded web UI
│   └── sketch.json    # Arduino config
├── hardware/          # Wiring, schematics, PCB (future)
├── enclosure/         # 3D printable case (coming soon)
├── docs/              # Documentation
│   ├── assembly.md    # Build instructions
│   └── images/        # Photos & screenshots
├── LICENSE            # MIT License
├── CONTRIBUTING.md    # How to contribute
└── README.md          # This file
```

## Building

### Requirements

- Arduino IDE or arduino-cli
- ESP32 board support package (v3.x)
- [ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard) library

### Library Modifications Required

The ESP32-BLE-Keyboard library needs modification to coexist with USBHIDKeyboard. Edit `BleKeyboard.h`:

1. Wrap KEY_* constants with `#ifndef KEY_LEFT_CTRL` guard
2. Wrap KeyReport typedef with `#ifndef _KEYREPORT_DEFINED` guard

Also edit `USBHIDKeyboard.h` in the ESP32 core library:
- Wrap KeyReport typedef with `#ifndef _KEYREPORT_DEFINED` guard

See [docs/library-patches.md](docs/library-patches.md) for details.

### Compile & Upload

**ESP32-S3 (XIAO):**
```bash
cd firmware
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default firmware.ino
arduino-cli upload -p /dev/cu.usbmodem1101 --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default firmware.ino
```

**CRITICAL**: Must use `USBMode=default` (TinyUSB) and `CDCOnBoot=default` (Enabled) for Web Serial API compatibility.
See [Programming Guide](docs/programming-guide.md) for detailed explanation.

**ESP32-C6:**
```bash
cd firmware
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C6:PartitionScheme=huge_app .
arduino-cli upload -p /dev/cu.usbmodem* --fqbn esp32:esp32:XIAO_ESP32C6:PartitionScheme=huge_app .
```

**Note**: If the USB port disappears, hold BOOT button while plugging in USB.

## Usage

### Normal Operation

1. Power on the device (blue breathing LED = WiFi on, LED off = WiFi off)
2. Touch enrolled finger on sensor
3. Password is typed automatically via USB or Bluetooth keyboard

### Configuration Mode

**Option 1: WiFi (Web Interface)**
1. Connect to WiFi network "TouchPass" (password: `touchpass`)
2. Open http://192.168.4.1 in any browser
3. Tap a finger on the hand diagram to enroll or edit
4. Set name, password, and optional "press Enter" setting

**Option 2: USB Serial Configuration (Recommended)**
1. Connect via USB-C cable to your computer
2. Open `firmware/config.html` in Chrome or Edge
3. Click "Connect Device" and select TouchPass
4. Visual interface for enrollment and configuration
5. No WiFi interference with fingerprint sensor
6. Uses Web Serial API (see [docs/serial-configuration.md](docs/serial-configuration.md))

### Toggle WiFi

Hold any finger on sensor for 5 seconds to toggle WiFi on/off.

### Switch Keyboard Mode (USB/BLE)

1. Open web interface at http://192.168.4.1
2. Click on the keyboard status in the top bar (shows "USB" or "BLE")
3. Confirm the switch - device will restart

### LED Indicators

| Pattern | Color | Meaning |
|---------|-------|---------|
| Breathing | Blue | WiFi enabled, standby |
| Off | - | WiFi disabled, standby |
| Breathing | Blue | Enrolling - waiting for finger |
| Flash | Green | Capture success |
| Solid | Green | Finger recognized |
| Solid | Red | Unknown finger / error |

## Platform Differences

| Feature | ESP32-S3 | ESP32-C6 |
|---------|----------|----------|
| USB HID Keyboard | ✓ | ✗ |
| BLE Keyboard | ✓ | ✓ |
| WiFi | ✓ (needs antenna) | ✓ (built-in) |
| Default Mode | USB | BLE |

**ESP32-S3 WiFi Note**: The XIAO ESP32-S3 requires an external antenna connected to the U.FL connector for reliable WiFi.

## Contributing

Contributions welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

- Report bugs or suggest features via Issues
- Submit PRs for code improvements
- Design a 3D printable case
- Improve documentation

## Security Notes

- Passwords stored in plain text in flash memory
- WiFi configuration requires physical proximity
- Standard USB HID / BLE HID (no encryption)
- Physical device access = password access
- Consider disabling WiFi after configuration (hold finger 5s)

## License

[MIT](LICENSE)
