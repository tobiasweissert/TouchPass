# Hardware

## Components

| Component | Description | Quantity |
|-----------|-------------|----------|
| Seeed XIAO ESP32-C6 | Microcontroller with WiFi 6 & BLE 5 | 1 |
| R502-A | Capacitive fingerprint sensor module | 1 |

## Wiring Diagram

```
R502-A Fingerprint Sensor          XIAO ESP32-C6
┌──────────────────────┐          ┌──────────────┐
│                      │          │              │
│  Pin 1 (VCC)  ───────┼──────────┼─── 3V3       │
│  Pin 2 (GND)  ───────┼──────────┼─── GND       │
│  Pin 3 (TXD)  ───────┼──────────┼─── D7 (RX)   │
│  Pin 4 (RXD)  ───────┼──────────┼─── D6 (TX)   │
│  Pin 5 (IRQ)  ───────┼──────────┼─── D2 (opt)  │
│  Pin 6 (VT)   ───────┼──────────┼─── 3V3       │
│                      │          │              │
└──────────────────────┘          └──────────────┘
```

## Pin Mapping

| R502-A Pin | Function | ESP32-C6 Pin | GPIO |
|------------|----------|--------------|------|
| 1 (VCC)    | Power 3.3V | 3V3 | - |
| 2 (GND)    | Ground | GND | - |
| 3 (TXD)    | Sensor TX → ESP RX | D7 | GPIO17 |
| 4 (RXD)    | Sensor RX ← ESP TX | D6 | GPIO16 |
| 5 (IRQ)    | Finger detect (optional) | D2 | GPIO4 |
| 6 (VT)     | Touch power | 3V3 | - |

## Notes

- The R502-A operates at 3.3V logic level, compatible with ESP32-C6
- Serial communication runs at 57600 baud
- The sensor has a built-in LED ring for status indication

## Future

This folder will contain:
- KiCad schematics
- PCB designs
- Gerber files for manufacturing
