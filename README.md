# m5-clock

NTP-synchronized digital clock for M5Stack Core2.

Displays date, weekday, and time with seconds. Syncs via WiFi/NTP at
startup and hourly, with RTC backup when WiFi is unavailable. Features
auto brightness adjustment (night mode) and SD card configuration.

## Hardware

- **M5Stack Core2** (ESP32, LCD 320x240, RTC, PSRAM, battery)

## Features

- Date (YYYY-MM-DD), weekday, and time (HH:MM:SS) display
- NTP time sync at startup + hourly intervals
- RTC backup when WiFi unavailable
- Night mode with auto brightness (configurable hours)
- Battery percentage display
- Static IP or DHCP
- SD card JSON configuration
- Button A: show config, Button C: manual NTP sync

## Setup

### Arduino IDE

1. Install board: **M5Stack** via Board Manager (v3.x)
2. Install libraries:
   - `M5Unified`
   - `ArduinoJson`
3. Open `m5-clock/m5-clock.ino`
4. Select board: **M5Stack-Core2**
5. Upload

### SD Card Configuration

Copy `config.json.example` to SD card as `/config.json` and edit:

```json
{
  "wifi": {
    "ssid": "your_ssid",
    "password": "your_password"
  },
  "ntp": {
    "server": "pool.ntp.org",
    "timezone": 9,
    "daylight_offset_sec": 0
  },
  "static_ip": {
    "enabled": false,
    "ip": "192.168.1.123",
    "gateway": "192.168.1.1",
    "subnet": "255.255.255.0",
    "dns1": "8.8.8.8"
  },
  "night_mode": {
    "enabled": true,
    "start_hour": 22,
    "end_hour": 7,
    "brightness": 30,
    "day_brightness": 200
  }
}
```

The clock works without an SD card using default settings (UTC+9, pool.ntp.org).

## Architecture

```
m5-clock/
├── m5-clock.ino         ← main sketch (setup/loop)
├── config.h             ← build-time constants
├── config_manager.h     ← SD card JSON config reader
├── ntp_sync.h           ← WiFi connection + NTP sync
└── display_manager.h    ← clock/config display (sprite double-buffered)
```

## Migration from v1.x

v2.0 migrates from the legacy `M5Core2.h` library to `M5Unified`:
- Requires ESP32 board package v3.x
- Remove legacy `M5Core2` library if installed
- Install `M5Unified` via Library Manager

## License

See [LICENSE](LICENSE).
