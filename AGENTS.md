# AGENTS.md — m5-clock

## Project summary

NTP-synchronized digital clock for M5Stack Core2. WiFi/NTP time sync
with RTC backup, night mode, SD card config. Part of lab-series.

## Development

- **IDE:** Arduino IDE
- **Board:** M5Stack-Core2 (board package v3.x)
- **Libraries:** M5Unified, ArduinoJson

## Key structure

```
m5-clock/
├── m5-clock/
│   ├── m5-clock.ino       ← main sketch
│   ├── config.h           ← build-time constants
│   ├── config_manager.h   ← SD card JSON config reader
│   ├── ntp_sync.h         ← WiFi + NTP sync
│   └── display_manager.h  ← clock/config display (M5Canvas sprite)
├── config.json.example    ← SD card config template
└── README.md / README.ja.md
```

## Configuration

Runtime config from SD card `/config.json`:
- `wifi.ssid`, `wifi.password` — WiFi credentials
- `ntp.server`, `ntp.timezone`, `ntp.daylight_offset_sec` — NTP settings
- `static_ip.enabled`, `.ip`, `.gateway`, `.subnet`, `.dns1` — optional static IP
- `night_mode.enabled`, `.start_hour`, `.end_hour`, `.brightness`, `.day_brightness`

Falls back to defaults (UTC+9, pool.ntp.org) without SD card.

## Gotchas

- M5Stack Core2 has PSRAM → sprite double-buffering works (unlike Basic v2.7)
- M5Unified: `M5.Rtc.setDateTime()` uses `m5::rtc_datetime_t` struct
- M5Unified: `M5.Power.getBatteryLevel()` returns 0-100 directly
- ESP32 board package v3.x required for M5Unified
- `NetworkManager` class name conflicts with ESP32 v3.x (not used here)
