# TouchPass

A hardware password manager that types your passwords when you touch it with an enrolled finger.

## Hardware

- **Microcontroller**: Seeed XIAO ESP32-C6
- **Fingerprint Sensor**: R502-A capacitive fingerprint module
- **Communication**: BLE HID Keyboard

### Wiring

| R502-A Pin | Function | ESP32-C6 Pin |
|------------|----------|--------------|
| 1 (VCC)    | Power    | 3V3          |
| 2 (GND)    | Ground   | GND          |
| 3 (TXD)    | TX       | D7 (GPIO17)  |
| 4 (RXD)    | RX       | D6 (GPIO16)  |
| 5 (IRQ)    | Interrupt| D2 (GPIO4) - Optional |
| 6 (VT)     | Touch    | 3V3          |

## Features

- **6-capture enrollment** - Touch ID style enrollment with 6 fingerprint captures for better accuracy
- **BLE Keyboard** - Types passwords automatically when finger is recognized
- **Web UI** - Configure via WiFi AP with a mobile-friendly interface
- **10 finger support** - Map passwords to specific fingers on left/right hands
- **Re-enrollment protection** - Re-enrolling a finger automatically overwrites the previous entry
- **Long-press WiFi toggle** - Hold finger for 5 seconds to enable/disable WiFi config mode

## Usage

### Normal Operation

1. Power on the device
2. Pair with your computer/phone via Bluetooth (device name: `TouchPass`)
3. Place an enrolled finger on the sensor
4. Password is automatically typed

### Configuration Mode

1. **Enable WiFi**: Hold any finger on the sensor for 5 seconds
   - LED will show blue breathing pattern when WiFi is active
2. **Connect**: Join the `TouchPass` WiFi network (password: `touchpass`)
3. **Configure**: Open `http://192.168.4.1` in your browser
4. **Disable WiFi**: Hold finger for 5 seconds again (or WiFi stays off by default on boot)

### Enrolling a Finger

1. In the web UI, tap a finger on the hand diagram
2. Click "Enroll"
3. Enter a name and password
4. Follow the 6-step capture process:
   - Steps 1-3: Place finger centered
   - Steps 4-6: Adjust grip to capture edges
5. LED feedback:
   - Blue breathing: Ready for capture
   - Green flash: Capture successful
   - Cyan flash: Phase 2 (edges) starting

### LED Indicators

| Pattern | Color | Meaning |
|---------|-------|---------|
| Breathing | Blue | WiFi config mode active |
| Solid | Green | Finger recognized |
| Solid | Red | Unknown finger or error |
| Flash | Green | Enrollment capture success |
| Off | - | Normal standby (WiFi off) |

## Building

### Requirements

- Arduino IDE or arduino-cli
- ESP32 board support package
- Libraries:
  - `BleKeyboard` (ESP32 BLE Keyboard)
  - Built-in: `WiFi`, `WebServer`, `Preferences`

### Compile & Upload

```bash
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C6:PartitionScheme=huge_app .
arduino-cli upload -p /dev/cu.usbmodem* --fqbn esp32:esp32:XIAO_ESP32C6:PartitionScheme=huge_app .
```

## File Structure

```
TouchPass/
├── TouchPass.ino    # Main firmware (sensor, BLE, web server)
├── webpage.h        # Embedded web UI (HTML/CSS/JS)
├── sketch.json      # Arduino IDE configuration
└── README.md        # This file
```

## API Endpoints

| Endpoint | Description |
|----------|-------------|
| `GET /` | Web UI |
| `GET /status` | Sensor status, enrolled count |
| `GET /fingers` | List enrolled fingers |
| `GET /finger/get?id=N` | Get finger details |
| `GET /finger/update?id=N&name=X&password=X` | Update finger |
| `GET /enroll/start?name=X&finger=N&password=X` | Start enrollment |
| `GET /enroll/status` | Poll enrollment progress |
| `GET /enroll/cancel` | Cancel enrollment |
| `GET /delete?id=N` | Delete finger |
| `GET /empty` | Delete all fingers |
| `GET /ble/status` | BLE connection status |
| `GET /detect` | Poll for finger detection events |

## Storage

Data is stored in ESP32 Preferences (NVS flash):

| Key Pattern | Data |
|-------------|------|
| `f{slot}` | Finger name |
| `p{slot}` | Password |
| `e{slot}` | Press Enter flag |
| `i{slot}` | Finger ID (0-9) mapping |

## Finger ID Mapping

| ID | Finger |
|----|--------|
| 0 | Left Index |
| 1 | Left Middle |
| 2 | Left Ring |
| 3 | Left Pinky |
| 4 | Left Thumb |
| 5 | Right Index |
| 6 | Right Middle |
| 7 | Right Ring |
| 8 | Right Pinky |
| 9 | Right Thumb |

## Security Notes

- Passwords are stored in plain text in flash memory
- WiFi AP has a simple password - intended for local configuration only
- No encryption on BLE keyboard transmission (standard HID)
- Physical access to device = access to passwords

## License

MIT
