# Changelog

All notable changes to TouchPass will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-01-14

### Breaking Changes

- **WiFi AP configuration mode removed** - Device no longer creates a WiFi access point
- **Web Serial API required** - Configuration now requires Chrome 89+ or Edge 89+
- **USB connection required** - Must connect via USB cable for configuration
- **No mobile configuration** - Mobile browsers do not support Web Serial API

### Changed

- **Serial-based configuration** - Replaced WiFi AP with USB serial communication at 115200 baud
- **Single-file configurator** - New `config.html` works offline, no internet required
- **JSON protocol** - Communication uses JSON command/response format over serial
- **Removed WiFi toggle** - No longer need to hold finger for 5 seconds to enter config mode
- **Always ready** - Device ready for serial commands whenever USB is connected
- **LED behavior** - Blue breathing now only shows during enrollment (not WiFi mode)

### Added

- `firmware/config.html` - Standalone Web Serial configurator
- `firmware/SerialCommandHandler.h` - JSON serial protocol implementation
- ArduinoJson library dependency
- Browser compatibility check in configurator
- Connection status indicator in UI
- Automatic reconnection handling

### Removed

- WiFi AP mode and all WiFi-related code
- WebServer library dependency
- `firmware/webpage.h` - Replaced by config.html
- 5-second hold WiFi toggle feature
- WiFi LED indicators (blue breathing for config mode)
- Network-based configuration

### Fixed

- Reduced memory usage by ~50KB RAM and ~100KB Flash
- Simplified device operation (no WiFi configuration needed)
- Improved security (no WiFi password, requires physical USB access)

### Migration

Existing users upgrading from v1.x:

1. **Flash new firmware** - Upload v2.0.0 to your device
2. **Data preserved** - All enrolled fingerprints and passwords are kept
3. **Reconfigure via USB** - Connect USB cable and open `config.html` in Chrome/Edge
4. **No WiFi needed** - Device will no longer start WiFi AP

Your enrolled fingers and passwords are stored in non-volatile memory and will not be lost during the upgrade.

## [1.0.0] - 2026-01-13

### Initial Release

- 6-capture Touch ID style enrollment
- BLE keyboard support (ESP32-C6)
- WiFi AP configuration mode
- Web interface for finger management
- 10 finger support (left/right hands)
- Re-enrollment protection
- Password storage per finger
- Auto-press Enter option
