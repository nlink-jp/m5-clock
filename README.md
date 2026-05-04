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

Copy `sdcard/config.example.json` to SD card as `/config.json` and edit:

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
    "brightness": 60,
    "day_brightness": 200
  }
}
```

| Field | Default | Description |
|-------|---------|-------------|
| `wifi.ssid` | (empty) | WiFi SSID |
| `wifi.password` | (empty) | WiFi password |
| `ntp.server` | `pool.ntp.org` | NTP server |
| `ntp.timezone` | `9` | UTC offset in hours |
| `ntp.daylight_offset_sec` | `0` | DST offset in seconds |
| `static_ip.enabled` | `false` | Use static IP instead of DHCP |
| `night_mode.enabled` | `true` | Enable auto brightness |
| `night_mode.start_hour` | `22` | Night mode start (24h) |
| `night_mode.end_hour` | `7` | Night mode end (24h) |
| `night_mode.brightness` | `60` | LCD brightness at night (0-255) |
| `night_mode.day_brightness` | `200` | LCD brightness during day (0-255) |

The clock works without an SD card using default settings (UTC+9, pool.ntp.org).

## Architecture

```
m5-clock/
├── m5-clock/
│   ├── m5-clock.ino         ← main sketch (setup/loop)
│   ├── config.h             ← build-time constants
│   ├── config_manager.h     ← SD card JSON config reader
│   ├── ntp_sync.h           ← WiFi + NTP sync (UTC internally, TZ at display)
│   └── display_manager.h    ← clock/config display (sprite double-buffered)
└── sdcard/
    └── config.example.json  ← SD card config template
```

## Technical Notes

- **Time handling:** Both system clock and RTC are stored as UTC end-to-end.
  Timezone offset is applied only at display time. TZ env is pinned to
  `UTC0` at boot, so `mktime`/`gmtime_r` are always interpreted as UTC.
- **Boot path:** System clock is seeded from RTC immediately at startup, so
  the clock view comes up showing the correct local time without waiting
  for NTP. Successful NTP syncs are written back to the RTC, so the clock
  survives power cycles.
- **Non-blocking sync:** WiFi connect + NTP request runs as a state
  machine driven from `loop()`; the clock keeps redrawing at 1 Hz while
  sync is in progress, with a `NTP syncing..` indicator in the footer.
- **Sync retry:** Failed NTP syncs retry with exponential backoff (1m, 2m,
  4m, … capped at the regular sync interval), not the full hourly period.
- **Night mode:** M5Unified correctly controls Core2 backlight brightness.
  The legacy M5Core2.h `setBrightness()` did not work reliably.
- **SD card:** Core2 uses GPIO4 as SD card CS pin. Explicit `SD.begin(4)`
  is required.

## Migration from v1.x

v2.0 migrates from the legacy `M5Core2.h` library to `M5Unified`:
- Requires ESP32 board package v3.x
- Remove legacy `M5Core2` library if installed
- Install `M5Unified` via Library Manager

## License

See [LICENSE](LICENSE).
