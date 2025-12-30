# Assembly Guide

Step-by-step instructions to build your TouchPass device.

## Prerequisites

### Tools Needed
- Soldering iron (optional, for permanent connections)
- Wire strippers
- Jumper wires (female-to-female recommended)

### Components
- 1x Seeed XIAO ESP32-C6
- 1x R502-A Fingerprint Sensor
- 6x Jumper wires

## Step 1: Wiring

Connect the R502-A sensor to the ESP32-C6:

| R502-A Wire | Color (typical) | ESP32-C6 Pin |
|-------------|-----------------|--------------|
| Pin 1 (VCC) | Red | 3V3 |
| Pin 2 (GND) | Black | GND |
| Pin 3 (TXD) | Yellow | D7 |
| Pin 4 (RXD) | Green | D6 |
| Pin 5 (IRQ) | Blue | D2 (optional) |
| Pin 6 (VT)  | White | 3V3 |

> **Note**: Wire colors may vary by manufacturer. Always verify with your sensor's datasheet.

## Step 2: Flash Firmware

1. Install Arduino IDE or arduino-cli
2. Install ESP32 board support
3. Install the BleKeyboard library
4. Open `firmware/firmware.ino`
5. Select board: **XIAO ESP32-C6**
6. Select partition scheme: **Huge APP**
7. Upload

```bash
# Or using arduino-cli:
cd firmware
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C6:PartitionScheme=huge_app .
arduino-cli upload -p /dev/cu.usbmodem* --fqbn esp32:esp32:XIAO_ESP32C6:PartitionScheme=huge_app .
```

## Step 3: First Boot

1. Power on via USB-C
2. The sensor LED should briefly flash green (sensor OK)
3. The device is now in standby mode

## Step 4: Configuration

1. Hold any finger on sensor for 5 seconds
2. LED starts blue breathing = WiFi enabled
3. Connect to `TouchPass` WiFi (password: `touchpass`)
4. Open http://192.168.4.1
5. Enroll your first finger

## Step 5: Pairing

1. On your computer/phone, search for Bluetooth devices
2. Pair with `TouchPass`
3. Test by scanning an enrolled finger - it should type the password

## Troubleshooting

### Sensor not responding
- Check wiring, especially TX/RX (they cross over)
- Verify 3.3V power on both VCC and VT pins

### WiFi not starting
- Hold finger for full 5 seconds
- Try with an enrolled finger

### Bluetooth not typing
- Ensure device is paired
- Check that a text field is focused
- Re-pair if needed
