# Arduino IDE Setup for ESP32-S3 (Critical!)

**IMPORTANT:** This configuration is **REQUIRED** for ESP32-S3 to work properly with TouchPass. Without these settings, you will experience no USB serial output and USB HID conflicts.

## Problem

ESP32-S3 cannot have both USB HID keyboard and USB CDC serial boot enabled simultaneously. The firmware manually initializes both interfaces as a composite USB device, but this requires specific Arduino IDE settings.

## Required Configuration Steps

### 1. Open Arduino IDE

Launch Arduino IDE with your TouchPass firmware project loaded.

### 2. Select the Correct Board

Go to: **Tools → Board → esp32 → XIAO_ESP32S3**

### 3. Critical Setting: USB CDC On Boot

Go to: **Tools → USB CDC On Boot → Disabled**

**This is the most important setting!** If this is enabled, USB HID and USB CDC will conflict.

### 4. Verify Other Settings

Ensure these settings are configured:

| Setting | Value | Notes |
|---------|-------|-------|
| **USB CDC On Boot** | **Disabled** | **CRITICAL - Must be disabled!** |
| USB Mode | Hardware CDC and JTAG | Enables both HID and CDC |
| Partition Scheme | 8MB with default | Enough space for firmware |
| Upload Speed | 921600 | Fast upload speed |
| CPU Frequency | 240MHz (WiFi) | Default for WiFi operation |
| Flash Mode | QIO 80MHz | Default flash mode |
| Flash Size | 8MB (64Mb) | XIAO ESP32-S3 has 8MB flash |

## Screenshot of Correct Settings

```
Tools Menu:
├── Board: "XIAO_ESP32S3"
├── USB CDC On Boot: "Disabled"          ← CRITICAL!
├── USB Mode: "Hardware CDC and JTAG"
├── Partition Scheme: "8MB with default"
└── Upload Speed: "921600"
```

## Verification

After uploading firmware with correct settings:

1. Disconnect and reconnect USB cable
2. Open Serial Monitor at 115200 baud
3. You should immediately see:
   ```json
   {"status":"ready","device":"TouchPass","mode":"usb"}
   ```

## Troubleshooting

### No Serial Output

**Symptom:** Serial monitor shows nothing when connecting USB

**Solution:**
1. Check **Tools → USB CDC On Boot → Disabled** (most common issue)
2. Try a different USB cable (some are power-only)
3. Try a different USB port on your computer
4. Verify board selection: **Tools → Board → XIAO_ESP32S3**
5. Check serial monitor baud rate: **Must be 115200**

### Still No Output After Checking Settings

If you've verified all settings and still see no output:

1. **Enter bootloader mode:**
   - Unplug USB
   - Hold BOOT button on XIAO board
   - Plug in USB while holding BOOT
   - Release BOOT after 1 second

2. **Re-upload with verbose output:**
   - File → Preferences → Show verbose output during: **upload** ✓
   - Sketch → Upload
   - Check console for errors

3. **Hardware test:**
   - Upload a simple test sketch (Blink) to verify board works
   - If Blink works but TouchPass doesn't, review settings again

### USB Port Disappears

**Symptom:** USB port disappears after uploading firmware

**Cause:** Firmware crashed or invalid USB configuration

**Solution:**
1. Follow bootloader mode steps above
2. Re-upload firmware with correct settings
3. Ensure "USB CDC On Boot: Disabled" is set

## Why This Matters

### Technical Explanation

**With "USB CDC On Boot: Enabled" (WRONG):**
- ESP32-S3 ROM bootloader creates USB CDC interface at boot
- TouchPass firmware tries to create USB HID keyboard
- Both interfaces conflict → USB stack fails
- Result: No serial output, keyboard doesn't work

**With "USB CDC On Boot: Disabled" (CORRECT):**
- ESP32-S3 ROM bootloader does not create USB CDC at boot
- TouchPass firmware initializes both USB HID and USB CDC manually
- They coexist as a composite USB device
- Result: Both serial and keyboard work perfectly

### Composite USB Device Architecture

After proper configuration, TouchPass creates:

```
USB Composite Device (Single USB Connection)
├── USB HID Keyboard Interface (password typing)
└── USB CDC Serial Interface (configuration @ 115200)
```

## Quick Reference Card

**Print this and keep it handy!**

```
╔════════════════════════════════════════════╗
║   ESP32-S3 TOUCHPASS - CRITICAL SETTING   ║
╠════════════════════════════════════════════╣
║                                            ║
║   Tools → USB CDC On Boot → Disabled      ║
║                                            ║
║   ⚠️  MUST BE DISABLED FOR TOUCHPASS! ⚠️   ║
║                                            ║
╚════════════════════════════════════════════╝
```

## Additional Resources

- [ESP32-S3 Migration Guide](esp32-s3-migration.md) - Full migration details
- [Hardware README](../hardware/README.md) - Wiring and pinout
- [Serial Configuration](serial-configuration.md) - USB Serial API usage

## Support

If you're still experiencing issues after following this guide:

1. Run diagnostics via Web Serial interface: `{"cmd":"diagnostics"}`
2. Check the troubleshooting section in [esp32-s3-migration.md](esp32-s3-migration.md)
3. Open an issue on GitHub with:
   - Your Arduino IDE settings screenshot
   - Serial monitor output (if any)
   - Diagnostics output (if available)
