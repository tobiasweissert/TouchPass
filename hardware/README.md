# Hardware

## Components

| Component | Description | Quantity |
|-----------|-------------|----------|
| Seeed XIAO ESP32-S3 | Recommended - USB HID + BLE | 1 |
| Seeed XIAO ESP32-C6 | Alternative - BLE only | 1 |
| R502-A | Capacitive fingerprint sensor module | 1 |
| External WiFi Antenna | Required for ESP32-S3 | 1 |

## Wiring Diagram

```
R502-A Fingerprint Sensor          XIAO ESP32-S3
┌──────────────────────┐          ┌──────────────┐
│                      │          │              │
│  Pin 1 (VCC)  Red   ─┼──────────┼─── 3V3       │
│  Pin 2 (GND)  Black ─┼──────────┼─── GND       │
│  Pin 3 (TXD)  Yellow┼──────────┼─── D5 (RX)   │
│  Pin 4 (RXD)  Green ─┼──────────┼─── D4 (TX)   │
│  Pin 5 (IRQ)  Blue  ─┼──────────┼─── (unused)  │
│  Pin 6 (VT)   White ─┼──────────┼─── 3V3       │
│                      │          │              │
└──────────────────────┘          └──────────────┘

Configuration Interface: USB cable (UART0 on GPIO43/44 @ 115200 baud)
Note: Arduino IDE must have "USB CDC On Boot: Disabled" to avoid USB HID conflicts
```

## Pin Mapping

The silkscreen labels map to different GPIOs on each chip:

**Note for ESP32-S3:** Using D4/D5 (GPIO5/6) for fingerprint sensor. USB CDC requires "USB CDC On Boot: Disabled" in Arduino IDE.

### ESP32-S3

| Pin/Function | Description | ESP32-S3 Pin | GPIO |
|--------------|-------------|--------------|------|
| **Fingerprint Sensor (UART1)** | | | |
| VCC (Red) | Power 3.3V | 3V3 | - |
| GND (Black) | Ground | GND | - |
| TXD (Yellow) | Sensor TX → ESP RX | D5 | GPIO6 |
| RXD (Green) | Sensor RX ← ESP TX | D4 | GPIO5 |
| IRQ (Blue) | Finger detect | - | (unused) |
| VT (White) | Touch power | 3V3 | - |
| **Configuration Interface** | | | |
| USB Serial | UART0 (USB CDC disabled) | USB-C Port | GPIO43/44 |
| USB HID Keyboard | Native USB | USB-C Port | GPIO19/20 |

### ESP32-C6

| Pin/Function | Description | ESP32-C6 Pin | GPIO |
|--------------|-------------|--------------|------|
| **Fingerprint Sensor (UART1)** | | | |
| VCC (Red) | Power 3.3V | 3V3 | - |
| GND (Black) | Ground | GND | - |
| TXD (Yellow) | Sensor TX → ESP RX | D7 | GPIO17 |
| RXD (Green) | Sensor RX ← ESP TX | D6 | GPIO16 |
| IRQ (Blue) | Finger detect | - | (unused) |
| VT (White) | Touch power | 3V3 | - |
| **Configuration Interface** | | | |
| USB Serial | Native USB CDC | USB-C Port | GPIO19/20 |

## R502-A Wire Colors

| Wire Color | Pin | Function |
|------------|-----|----------|
| Red | 1 | VCC (3.3V) |
| Black | 2 | GND |
| Yellow | 3 | TXD (sensor output) |
| Green | 4 | RXD (sensor input) |
| Blue | 5 | IRQ (touch interrupt) |
| White | 6 | VT (touch power) |

## Platform Comparison

| Feature | ESP32-S3 | ESP32-C6 |
|---------|----------|----------|
| USB HID Keyboard | ✓ Native | ✗ |
| BLE Keyboard | ✓ | ✓ |
| WiFi | ✓ External antenna | ✓ Built-in |
| Price | ~$7 | ~$5 |
| Recommended | Yes | Budget option |

## ESP32-S3 Notes

### External Antenna Required

The XIAO ESP32-S3 has weak built-in WiFi. You **must** connect the external antenna to the U.FL connector on the bottom-left of the board for WiFi to work.

### USB Port Recovery

If the firmware crashes and USB port disappears:
1. Unplug USB
2. Hold BOOT button
3. Plug in USB while holding BOOT
4. Release BOOT
5. Flash new firmware

## R502-A Sensor Notes

- Operating voltage: 3.3V DC
- Operating current: ~30mA active, ~2μA touch detect
- Serial baud rate: 57600
- Storage capacity: 200 fingerprints
- Built-in RGB LED ring for status feedback
- Capacitive sensing (works through thin materials)

## Future

This folder will contain:
- KiCad schematics
- PCB designs
- Gerber files for manufacturing
