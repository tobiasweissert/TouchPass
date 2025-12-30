# TouchPass

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A hardware password manager that types your passwords when you touch it with an enrolled finger.

## Features

- **6-capture enrollment** - Touch ID style enrollment for better accuracy
- **BLE Keyboard** - Types passwords automatically when finger is recognized
- **Web UI** - Configure via WiFi with a mobile-friendly interface
- **10 finger support** - Map passwords to specific fingers on left/right hands
- **Re-enrollment protection** - Re-enrolling a finger overwrites the previous entry
- **Long-press WiFi toggle** - Hold finger 5 seconds to enable/disable config mode

## Hardware

| Component | Description |
|-----------|-------------|
| Seeed XIAO ESP32-C6 | Microcontroller with WiFi 6 & BLE 5 |
| R502-A | Capacitive fingerprint sensor |

See [hardware/README.md](hardware/README.md) for wiring diagram and details.

## Quick Start

1. **Build** - See [docs/assembly.md](docs/assembly.md)
2. **Flash** - Upload firmware from `firmware/`
3. **Pair** - Connect to `TouchPass` via Bluetooth
4. **Configure** - Hold finger 5s → Join `TouchPass` WiFi → http://192.168.4.1
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
- ESP32 board support package
- [BleKeyboard](https://github.com/T-vK/ESP32-BLE-Keyboard) library

### Compile & Upload

```bash
cd firmware
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C6:PartitionScheme=huge_app .
arduino-cli upload -p /dev/cu.usbmodem* --fqbn esp32:esp32:XIAO_ESP32C6:PartitionScheme=huge_app .
```

## Usage

### Normal Operation

1. Power on the device
2. Pair via Bluetooth (device: `TouchPass`)
3. Place enrolled finger on sensor
4. Password is typed automatically

### Configuration Mode

1. Hold any finger for 5 seconds (LED turns blue)
2. Connect to `TouchPass` WiFi (password: `touchpass`)
3. Open http://192.168.4.1
4. Enroll fingers and set passwords

### LED Indicators

| Pattern | Color | Meaning |
|---------|-------|---------|
| Breathing | Blue | WiFi config mode |
| Solid | Green | Finger recognized |
| Solid | Red | Unknown finger |
| Flash | Green | Capture success |
| Off | - | Standby |

## Contributing

Contributions welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

- Report bugs or suggest features via Issues
- Submit PRs for code improvements
- Design a 3D printable case
- Improve documentation

## Security Notes

- Passwords stored in plain text in flash
- WiFi AP uses simple password (local config only)
- Standard BLE HID (no encryption)
- Physical access = password access

## License

[MIT](LICENSE)
