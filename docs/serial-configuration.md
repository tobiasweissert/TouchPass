# Serial Configuration Guide

TouchPass supports both WiFi and USB Serial configuration. USB Serial allows you to configure the device without WiFi interference with the fingerprint sensor.

## Hardware Setup

### ESP32-S3
Simply connect via USB-C cable to your computer:
- **USB-C Cable** → Computer
- No additional wiring required
- Uses UART0 on GPIO43/44 (accessed through USB-C)
- **Important**: Arduino IDE must have "USB CDC On Boot: Disabled" to avoid USB HID conflicts

### ESP32-C6
Simply connect via USB-C cable to your computer:
- **USB-C Cable** → Computer
- No additional wiring required
- Uses native USB CDC Serial (GPIO19/20 internally)

**Note**: USB Serial runs at **115200 baud**, 8N1.

## Serial Protocol

Commands are sent as JSON over serial, one command per line (terminated with `\n`).

### Command Format
```json
{"cmd": "command_name", "params": {...}, "id": 123}
```

- `cmd` (required): Command name
- `params` (optional): Command parameters as JSON object
- `id` (optional): Request ID for matching responses

### Response Format
```json
{"status": "ok", "data": {...}, "id": 123}
```

Or on error:
```json
{"status": "error", "message": "error description", "id": 123}
```

## Available Commands

### System Info
```bash
{"cmd": "get_system_info"}
```
Returns chip type, firmware version, free heap, and WiFi status.

### Get Status
```bash
{"cmd": "get_status"}
```
Returns sensor status, fingerprint count, and capacity.

### List Fingers
```bash
{"cmd": "get_fingers"}
```
Returns array of enrolled fingerprints with IDs and names.

### Enroll Finger
```bash
{"cmd": "enroll_start", "params": {"name": "Gmail", "password": "mypassword", "pressEnter": true, "finger": 0}}
```
- `name` (required): Display name for the fingerprint
- `password` (optional): Password to type when finger is detected
- `pressEnter` (optional): Press Enter after typing password (default: false)
- `finger` (optional): Finger ID 0-9 for mapping (0=left thumb, 1=left index, etc.)

Then poll enrollment status:
```bash
{"cmd": "enroll_status"}
```

### Get Finger Details
```bash
{"cmd": "get_finger", "params": {"id": 0}}
```
Returns name, password status, and press-enter setting for a specific slot.

### Update Finger
```bash
{"cmd": "update_finger", "params": {"id": 0, "name": "New Name", "password": "newpass", "pressEnter": false}}
```

### Delete Finger
```bash
{"cmd": "delete_finger", "params": {"id": 0}}
```

### Clear All Fingerprints
```bash
{"cmd": "empty_library"}
```

### Get Keyboard Status
```bash
{"cmd": "get_keyboard_mode"}
```
Returns current keyboard mode (USB or BLE).

### Set Keyboard Mode
```bash
{"cmd": "set_keyboard_mode", "params": {"mode": "usb"}}
```
Mode can be `"usb"` or `"ble"`. Device will restart after mode change.

### Reboot
```bash
{"cmd": "reboot"}
```

## Example: Enroll a Finger

```bash
# Start enrollment
{"cmd": "enroll_start", "params": {"name": "GitHub", "password": "ghp_token123", "pressEnter": true, "finger": 1}}

# Response:
# {"status":"ok","data":{"ok":true,"status":"Place finger on sensor"},"id":-1}

# Poll enrollment status (repeat until done)
{"cmd": "enroll_status"}

# Response (in progress):
# {"status":"ok","data":{"state":1,"totalSteps":6,"step":1,"captured":false,"phase":"center","message":"Place finger on sensor","done":false}}

# Response (completed):
# {"status":"ok","data":{"state":9,"totalSteps":6,"step":6,"captured":true,"phase":"edges","message":"","done":true,"ok":true,"name":"GitHub","status":"Enrolled successfully"}}
```

## Python Example

```python
import serial
import json
import time

# Open serial port
# Linux/Windows: '/dev/ttyACM0' or 'COM3'
# macOS: '/dev/cu.usbmodem*' (find with: ls /dev/cu.usbmodem*)
ser = serial.Serial('/dev/cu.usbmodem101', 115200, timeout=1)
time.sleep(2)  # Wait for device ready

def send_command(cmd, params=None, req_id=1):
    command = {"cmd": cmd, "id": req_id}
    if params:
        command["params"] = params
    ser.write((json.dumps(command) + '\n').encode())
    response = ser.readline().decode().strip()
    return json.loads(response)

# Get system info
info = send_command("get_system_info")
print(f"Chip: {info['data']['chip']}")
print(f"Firmware: {info['data']['firmware']}")

# List enrolled fingers
fingers = send_command("get_fingers")
print(f"Enrolled: {len(fingers['data']['fingers'])} fingers")

# Enroll new finger
result = send_command("enroll_start", {
    "name": "MyPassword",
    "password": "secret123",
    "pressEnter": True,
    "finger": 0
})
print(result['data']['status'])

# Poll enrollment
while True:
    status = send_command("enroll_status")
    data = status['data']
    print(f"Step {data['step']}/6: {data['message']}")
    if data['done']:
        print(f"Result: {data['status']}")
        break
    time.sleep(0.5)

ser.close()
```

## Web Serial API (Browser)

You can also configure TouchPass directly from a browser using the Web Serial API:

```html
<!DOCTYPE html>
<html>
<body>
  <button onclick="connect()">Connect to TouchPass</button>
  <button onclick="getStatus()">Get Status</button>
  <pre id="output"></pre>

  <script>
    let port;
    let reader;
    let writer;

    async function connect() {
      port = await navigator.serial.requestPort();
      await port.open({ baudRate: 115200 });

      const decoder = new TextDecoderStream();
      const readableStreamClosed = port.readable.pipeTo(decoder.writable);
      reader = decoder.readable.getReader();

      const encoder = new TextEncoderStream();
      const writableStreamClosed = encoder.readable.pipeTo(port.writable);
      writer = encoder.writable.getWriter();

      document.getElementById('output').textContent = 'Connected!';

      // Read responses
      readLoop();
    }

    async function readLoop() {
      while (true) {
        const { value, done } = await reader.read();
        if (done) break;
        document.getElementById('output').textContent = value;
      }
    }

    async function sendCommand(cmd, params) {
      const command = { cmd, params, id: Date.now() };
      await writer.write(JSON.stringify(command) + '\n');
    }

    async function getStatus() {
      await sendCommand('get_status');
    }
  </script>
</body>
</html>
```

## Advantages of USB Serial Configuration

1. **No WiFi interference** - ESP32-S3 uses UART0 (GPIO43/44) for config, fingerprint sensor on UART1 (GPIO40/41)
2. **No additional wiring** - Just plug in the USB-C cable you use for programming
3. **Works without WiFi** - Configure even with WiFi disabled
4. **Automation-friendly** - Easy to script with Python, Node.js, etc.
5. **Web Serial API** - Configure from browser without installing drivers
6. **Debugging** - View real-time responses and system info

## WiFi Configuration Still Available

The web interface at http://192.168.4.1 still works as before. Serial configuration is an additional option, not a replacement.
