# TouchPass Programming Guide

## Correct Firmware Upload Procedure

### Arduino CLI Command

Use this command to compile and upload firmware to the ESP32-S3:

```bash
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default firmware.ino
arduino-cli upload -p /dev/cu.usbmodem1101 --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default firmware.ino
```

### Critical Settings Explained

#### 1. **USBMode=default** (USB-OTG TinyUSB)
- **Required for Web Serial API compatibility**
- Uses software-based USB CDC implementation via TinyUSB
- Provides proper bidirectional communication
- **DO NOT use `USBMode=hwcdc`** (Hardware CDC and JTAG mode has issues with Web Serial API)

#### 2. **CDCOnBoot=default** (Enabled)
- Ensures USB Serial port remains available after firmware boots
- Without this, the device only appears in bootloader mode
- Required for the browser to maintain connection after reset

## Why This Matters

### USB Mode Comparison

| Mode | Value | Description | Web Serial API |
|------|-------|-------------|----------------|
| **Hardware CDC and JTAG** | `USBMode=hwcdc` | Uses built-in USB Serial/JTAG controller (hardware-based) | ❌ Has bidirectional communication issues |
| **USB-OTG (TinyUSB)** | `USBMode=default` | Software-based CDC via TinyUSB stack | ✅ Full compatibility |

### What You'll See

**Correct Setup (USB-OTG):**
- Device shows as "Serial Port (USB)" in Arduino CLI
- Browser can send commands and receive responses
- Bidirectional communication works perfectly

**Incorrect Setup (Hardware CDC):**
- Device shows as "USB JTAG/serial debug unit"
- Browser can send but cannot receive data
- `reader.read()` blocks forever
- Commands timeout after 5 seconds

## Step-by-Step Upload Process

### 1. Close Browser Connection
```bash
# The serial port must not be in use
# Close any browser tabs with config.html open
```

### 2. Check Device Connection
```bash
arduino-cli board list | grep -i usb
# Should show: /dev/cu.usbmodem1101  serial   Serial Port (USB)
```

### 3. Compile Firmware
```bash
cd firmware
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default firmware.ino
```

### 4. Upload Firmware
```bash
arduino-cli upload -p /dev/cu.usbmodem1101 --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default firmware.ino
```

### 5. Verify Device Reconnects
```bash
sleep 3
arduino-cli board list | grep -i usb
# Device should still be present at /dev/cu.usbmodem1101
```

### 6. Test with Browser
1. Open `config.html` in Chrome or Edge
2. Open browser console (Cmd+Option+I or F12)
3. Click "Connect Device"
4. Select "ESP32 Family Device" or similar
5. Should see debug messages and successful connection

## Quick Reference

### One-Line Compile + Upload
```bash
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default firmware.ino && arduino-cli upload -p /dev/cu.usbmodem1101 --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default firmware.ino
```

### Makefile (Optional)
Create a `Makefile` in the firmware directory:

```makefile
FQBN = esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default
PORT = /dev/cu.usbmodem1101

compile:
	arduino-cli compile --fqbn $(FQBN) firmware.ino

upload:
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) firmware.ino

all: compile upload

.PHONY: compile upload all
```

Then use:
```bash
make all
```

## Troubleshooting

### "Port is busy" Error
**Problem:** `Could not open /dev/cu.usbmodem1101, the port is busy`

**Solution:**
1. Close all browser tabs with config.html
2. Close Arduino Serial Monitor if open
3. Wait 2-3 seconds
4. Try upload again

### Device Disappears After Boot
**Problem:** Device only visible during bootloader mode

**Solution:** You're using wrong USB mode or CDC setting:
```bash
# Check current settings
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default --show-properties firmware.ino | grep -i "usb\|cdc"

# Should show:
# ARDUINO_USB_MODE=1
# ARDUINO_USB_CDC_ON_BOOT=1
```

### Command Timeouts in Browser
**Problem:** Browser sends commands but gets "Command timeout" after 5 seconds

**Symptoms:**
- `reader.read()` blocks forever
- No "📥 Received data" messages in console
- No loop iteration logs beyond #1

**Solution:** Wrong USB mode! Use `USBMode=default` (TinyUSB), NOT `USBMode=hwcdc`

### Monitor Serial Output (Debug)
To see what the ESP32 is outputting:
```bash
arduino-cli monitor -p /dev/cu.usbmodem1101 -c baudrate=115200
```

Press Ctrl+C to exit.

## Board Configuration Reference

Available options for XIAO_ESP32S3:

| Option | Recommended | Alternative |
|--------|-------------|-------------|
| **USB Mode** | `USBMode=default` (TinyUSB) | `USBMode=hwcdc` (Hardware CDC - don't use) |
| **USB CDC On Boot** | `CDCOnBoot=default` (Enabled) | `CDCOnBoot=cdc` (Disabled) |
| **Upload Speed** | `UploadSpeed=921600` (default) | 115200, 230400, 460800 |
| **CPU Frequency** | `CPUFreq=240` (default) | 160, 80, 40, 20, 10 |

## Web Serial API Requirements

- **Browser:** Chrome 89+, Edge 89+, or Opera (not Firefox or Safari)
- **Connection:** Must be HTTPS or localhost (file:// works)
- **USB Mode:** TinyUSB (`USBMode=default`)
- **CDC On Boot:** Enabled (`CDCOnBoot=default`)

## References

- [ESP32-S3 USB Serial/JTAG Controller](https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s3/api-guides/usb-serial-jtag-console.html)
- [ESP32-S3 USB OTG Console](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-otg-console.html)
- [USB Settings for ESP32-S3 in PlatformIO](https://thingpulse.com/usb-settings-for-logging-with-the-esp32-s3-in-platformio/)
- [Web Serial API Documentation](https://developer.mozilla.org/en-US/docs/Web/API/Serial)

## Summary

**Always use these exact settings for TouchPass:**
```
--fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default
```

This ensures:
- ✅ USB-OTG (TinyUSB) mode for Web Serial API compatibility
- ✅ USB CDC stays available after boot
- ✅ Bidirectional serial communication works
- ✅ Browser can send commands AND receive responses
