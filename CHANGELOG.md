# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [2.0.0] - 2026-04-14

### Changed

- Migrate from `M5Core2.h` to `M5Unified` (supports ESP32 board package v3.x)
- Refactor single .ino into modular architecture:
  - `config.h` — build-time constants
  - `config_manager.h` — SD card JSON config reader
  - `ntp_sync.h` — WiFi connection + NTP sync
  - `display_manager.h` — clock/config display
- Move sketch into `m5-clock/` subdirectory (Arduino convention)
- RTC API: `M5.Rtc.SetTime/SetDate` → `M5.Rtc.setDateTime`
- Power API: `M5.Axp.GetBatVoltage()` → `M5.Power.getBatteryLevel()`
- Display API: `M5.Lcd` → `M5.Display`, `TFT_eSprite` → `M5Canvas`
- ArduinoJson: `StaticJsonDocument<1024>` → `JsonDocument` (v7 compatible)
- Config load failure no longer hangs (`while(1)`) — falls back to defaults

### Added

- README.ja.md (Japanese documentation)
- AGENTS.md

## [1.0.0] - 2026-03-19

### Added

- Date, weekday, and time display (sprite double-buffered)
- SD card `config.json` configuration
- WiFi + NTP time sync (startup + hourly)
- RTC fallback when WiFi unavailable
- Static IP support
- Night mode (time-based brightness + red text)
- Battery percentage display
- NTP sync indicator
- Touch button controls (A: config, C: manual NTP sync)
- Config display screen (10s auto-close)
