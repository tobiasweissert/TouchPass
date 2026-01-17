# TouchPass Quick Reference Card

## Programming Command (Copy-Paste Ready)

### ESP32-S3 (XIAO)
```bash
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default firmware.ino && arduino-cli upload -p /dev/cu.usbmodem1101 --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default firmware.ino
```

### Critical Settings
- **USBMode=default** → USB-OTG (TinyUSB) - Required for Web Serial API
- **CDCOnBoot=default** → CDC Enabled - Required to stay connected after boot

## Common Issues & Fixes

| Problem | Solution |
|---------|----------|
| **"Port is busy"** | Close browser tab with config.html, wait 3 seconds, retry |
| **Device disappears after upload** | Wrong USB mode! Use `USBMode=default` not `USBMode=hwcdc` |
| **Command timeout in browser** | Wrong USB mode! Switch to TinyUSB (`USBMode=default`) |
| **Can't find device in browser** | Must use Chrome/Edge (not Firefox/Safari), use https:// or localhost |
| **"reader.read() blocking forever"** | Wrong USB mode - using Hardware CDC instead of TinyUSB |

## Upload Checklist

- [ ] Close all browser tabs with config.html
- [ ] Device showing at `/dev/cu.usbmodem1101`
- [ ] Using `USBMode=default` (TinyUSB)
- [ ] Using `CDCOnBoot=default` (Enabled)
- [ ] Upload successful
- [ ] Device reconnects after upload
- [ ] Open config.html in Chrome/Edge
- [ ] Can connect and see data in console

## Verification Commands

```bash
# Check device is connected
arduino-cli board list | grep -i usb

# Monitor serial output
arduino-cli monitor -p /dev/cu.usbmodem1101 -c baudrate=115200

# Verify settings during compile
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3:USBMode=default,CDCOnBoot=default --show-properties firmware.ino | grep -i "usb\|cdc"
```

## USB Mode Comparison

| Mode | Setting | Web Serial | Status |
|------|---------|------------|--------|
| **Hardware CDC** | `USBMode=hwcdc` | ❌ Broken | Don't use |
| **TinyUSB** | `USBMode=default` | ✅ Works | Use this |

## Browser Requirements

- ✅ Chrome 89+
- ✅ Edge 89+
- ✅ Opera
- ❌ Firefox (no Web Serial API)
- ❌ Safari (no Web Serial API)

## File Locations

- Firmware: `firmware/firmware.ino`
- Config UI: `firmware/config.html`
- Documentation: `docs/programming-guide.md`

## Expected Console Output (Success)

```
[Serial] Port opened successfully
[Serial] Text streams configured successfully
[Serial] === READ LOOP STARTING ===
[Serial] Loop iteration #1, waiting for data...
[Serial] 📥 Received data: {"debug":"Serial initialized"}
[Serial] ✓ Parsed JSON: {debug: "Serial initialized"}
[Serial] Sending command #1: get_status
[Serial] ✓ Command #1 write complete
[Serial] 📥 Received data: {"status":"ok","data":{...},"id":1}
[Serial] ✓ Command #1 SUCCESS
```

## Support

- Full guide: `docs/programming-guide.md`
- Issues: https://github.com/anthropics/TouchPass/issues (if public)
