# TouchPass

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A hardware password manager that types your passwords when you touch it with an enrolled finger.

## Features

- **6-capture enrollment** - Touch ID style enrollment for better accuracy
- **BLE Keyboard** - Types passwords automatically when finger is recognized
- **Web Serial Configuration** - Configure via USB using Chrome/Edge browser
- **10 finger support** - Map passwords to specific fingers on left/right hands
- **Re-enrollment protection** - Re-enrolling a finger overwrites the previous entry
- **Offline configurator** - Single HTML file, no internet required

## Hardware

| Component | Description |
|-----------|-------------|
| Seeed XIAO ESP32-C6 | Microcontroller with WiFi 6 & BLE 5 |
| R502-A | Capacitive fingerprint sensor |

See [hardware/README.md](hardware/README.md) for wiring diagram and details.

## Quick Start

1. **Build** - See [docs/assembly.md](docs/assembly.md)
2. **Flash** - Upload firmware from `firmware/`
3. **Configure** - Connect via USB → Open `firmware/config.html` in Chrome/Edge → Click "Connect Device"
4. **Pair** - Connect to `TouchPass` via Bluetooth
5. **Use** - Touch enrolled finger to type password

## Project Structure

```
TouchPass/
├── firmware/          # ESP32 source code
│   ├── firmware.ino   # Main firmware
│   ├── config.html    # Web Serial configurator (open in Chrome/Edge)
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
- [ArduinoJson](https://arduinojson.org/) library (v6+)

### Configuration Requirements

- Chrome 89+ or Edge 89+ (Web Serial API support)
- USB cable for device connection

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

1. Connect TouchPass to your computer via USB
2. Open `firmware/config.html` in Chrome or Edge browser
3. Click "Connect Device" and select TouchPass from the list
4. Enroll fingers and set passwords

**Note**: Web Serial API only works in Chrome/Edge. Firefox and Safari are not supported.

### LED Indicators

| Pattern | Color | Meaning |
|---------|-------|---------|
| Breathing | Blue | Enrolling finger |
| Solid | Green | Finger recognized |
| Solid | Red | Unknown finger |
| Flash | Green | Capture success |
| Flash | Cyan | Phase change (center → edges) |
| Off | - | Standby |

## Contributing

Contributions welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

- Report bugs or suggest features via Issues
- Submit PRs for code improvements
- Design a 3D printable case
- Improve documentation

## Security Notes

- Passwords stored in plain text in flash memory
- USB serial configuration requires physical access
- Standard BLE HID (no encryption)
- Physical device access = password access
- Configuration only possible via USB connection

## License

[MIT](LICENSE)
