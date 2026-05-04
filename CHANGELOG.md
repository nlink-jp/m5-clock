# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Fixed

- Clock display no longer drifts or sticks to a stale value after a failed
  periodic sync. Previously the sync routine called `settimeofday({0,0})`
  before issuing the NTP request, so any timeout silently zeroed the
  system clock and left the display showing whatever stale value the RTC
  happened to hold.
- Clock no longer freezes for ~15 s during periodic NTP sync. Sync now
  runs as a non-blocking state machine in `loop()`; the clock continues
  to redraw at 1 Hz while WiFi connects and the NTP response is awaited.
- Display fallback no longer reads RTC with an inconsistent timezone
  interpretation. `NtpSync.getLocalTime()` is now the single time source.

### Changed

- Time storage convention is fixed end-to-end as UTC: both `time()` and
  the M5Unified RTC store UTC, and the timezone offset is applied only at
  display time. TZ env is pinned to `UTC0` at boot.
- RTC is now seeded into the system clock at boot, and written back after
  every successful NTP sync. The clock therefore boots showing the
  correct local time immediately once RTC has been initialized at least
  once. The "Syncing time…" splash is gone; if RTC has never been set,
  the clock view shows `--:--:--` until the first sync completes.
- Failed NTP syncs retry with exponential backoff (1m, 2m, 4m, … capped
  at the regular sync interval) instead of waiting a full hour.
- NTP completion is now detected via `sntp_get_sync_status()` rather than
  by polling `getLocalTime()`, so transient successes/failures are
  reported deterministically.
- `WIFI_MAX_ATTEMPTS` / `WIFI_ATTEMPT_DELAY_MS` removed in favour of a
  single `WIFI_TIMEOUT_MS` total budget used by the state machine.

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
