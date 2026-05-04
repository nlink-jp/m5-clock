// ntp_sync.h — WiFi connection and NTP time sync (non-blocking state machine)
//
// Time storage convention (kept consistent end-to-end):
//   - System clock (time()) : always UTC
//   - RTC (M5.Rtc)          : always UTC
//   - Timezone offset is applied only at display time via _gmtOffset.
//
// Boot order: seed system clock from RTC → first sync runs in update().
// On success, the resulting UTC is written back to the RTC. Once RTC
// holds a valid value, the clock keeps ticking across power cycles and
// getLocalTime() never has to fall back to a stale RTC read.
//
// update() steps a state machine and returns immediately, so loop() can
// keep redrawing the clock at 1 Hz even while a sync is in progress.
#pragma once

#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <time.h>
#include <sys/time.h>
#include "config.h"
#include "config_manager.h"

class NtpSync {
public:
  NtpSync()
    : _gmtOffset(0),
      _state(SYNC_IDLE),
      _stateStartMs(0),
      _nextSyncMs(0),
      _failures(0) {
    strlcpy(_lastSyncStr, "--:--", sizeof(_lastSyncStr));
  }

  // Call once at setup, after config is loaded.
  void begin(const ClockConfig& cfg) {
    _gmtOffset = cfg.gmt_offset_sec + cfg.daylight_offset_sec;

    // Pin TZ so mktime/gmtime_r treat all struct tm values as UTC.
    setenv("TZ", "UTC0", 1);
    tzset();

    // Seed system clock from RTC (RTC is UTC). After this, time() is
    // valid even before any successful NTP sync.
    if (_seedSystemTimeFromRtc()) {
      Serial.println("[NTP] seeded system clock from RTC");
    } else {
      Serial.println("[NTP] RTC not initialized, awaiting NTP");
    }

    // Trigger first sync on the next update() call.
    _nextSyncMs = millis();
  }

  // Call from loop() every iteration. Steps the sync state machine by
  // one micro-step and returns immediately; never blocks for I/O.
  void update(const ClockConfig& cfg) {
    switch (_state) {
      case SYNC_IDLE:
        if ((long)(millis() - _nextSyncMs) >= 0) {
          _enterWifiConnect(cfg);
        }
        break;
      case SYNC_WIFI_WAITING:
        _stepWifi(cfg);
        break;
      case SYNC_NTP_WAITING:
        _stepNtp();
        break;
    }
  }

  // Schedules an immediate sync. Ignored if a sync is already in flight.
  void manualSync(const ClockConfig& /*cfg*/) {
    if (_state == SYNC_IDLE) {
      _nextSyncMs = millis();
    }
  }

  // Returns local time as struct tm. Returns false only when no time source
  // has ever been valid (cold boot with no RTC and no NTP yet).
  bool getLocalTime(struct tm& out) const {
    time_t now;
    time(&now);
    if (now < kMinValidEpoch) return false;
    now += _gmtOffset;
    gmtime_r(&now, &out);
    return true;
  }

  bool hasValidTime() const {
    time_t now;
    time(&now);
    return now >= kMinValidEpoch;
  }

  bool isSyncing() const { return _state != SYNC_IDLE; }
  const char* lastSyncTime() const { return _lastSyncStr; }

private:
  enum SyncState {
    SYNC_IDLE,
    SYNC_WIFI_WAITING,
    SYNC_NTP_WAITING,
  };

  // 2025-01-01 00:00:00 UTC. Anything below is "no valid time".
  static constexpr time_t kMinValidEpoch = 1735689600;
  // Base retry interval after a failed sync; doubles per consecutive failure.
  static constexpr unsigned long kRetryBaseMs = 60000UL;

  long _gmtOffset;
  SyncState _state;
  unsigned long _stateStartMs;
  unsigned long _nextSyncMs;
  int _failures;
  char _lastSyncStr[6];

  void _enterWifiConnect(const ClockConfig& cfg) {
    if (strlen(cfg.ssid) == 0) {
      Serial.println("[NTP] no SSID configured");
      _onFailure();
      return;
    }

    // Brief settling pauses (~100 ms total) — single missed frame at most.
    WiFi.disconnect(true);
    delay(50);
    WiFi.mode(WIFI_STA);
    delay(50);

    if (cfg.static_ip_enabled) {
      IPAddress ip, gw, sn, dns;
      if (ip.fromString(cfg.ip) && gw.fromString(cfg.gateway) &&
          sn.fromString(cfg.subnet) && dns.fromString(cfg.dns1)) {
        WiFi.config(ip, gw, sn, dns);
      }
    }

    Serial.printf("[NTP] connecting to %s\n", cfg.ssid);
    WiFi.begin(cfg.ssid, cfg.password);

    _state = SYNC_WIFI_WAITING;
    _stateStartMs = millis();
  }

  void _stepWifi(const ClockConfig& cfg) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[NTP] connected: %s\n",
                    WiFi.localIP().toString().c_str());
      _enterNtpRequest(cfg);
      return;
    }
    if (millis() - _stateStartMs >= WIFI_TIMEOUT_MS) {
      Serial.println("[NTP] WiFi connection failed");
      WiFi.disconnect(true);
      _onFailure();
    }
  }

  void _enterNtpRequest(const ClockConfig& cfg) {
    Serial.println("[NTP] requesting time (UTC)...");
    // configTime() internally calls sntp_stop() then sntp_init(), so
    // sntp_get_sync_status() resets to RESET and transitions to COMPLETED
    // when the response arrives.
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    configTime(0, 0, cfg.ntp_server);
    _state = SYNC_NTP_WAITING;
    _stateStartMs = millis();
  }

  void _stepNtp() {
    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
      time_t now;
      time(&now);
      if (now < kMinValidEpoch) {
        Serial.println("[NTP] response gave bogus time");
        WiFi.disconnect(true);
        _onFailure();
        return;
      }
      struct tm utc;
      gmtime_r(&now, &utc);
      Serial.printf("[NTP] UTC: %04d-%02d-%02d %02d:%02d:%02d\n",
                    utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                    utc.tm_hour, utc.tm_min, utc.tm_sec);

      _writeRtc(utc);

      time_t local = now + _gmtOffset;
      struct tm lt;
      gmtime_r(&local, &lt);
      snprintf(_lastSyncStr, sizeof(_lastSyncStr), "%02d:%02d",
               lt.tm_hour, lt.tm_min);

      WiFi.disconnect(true);
      _onSuccess();
      return;
    }
    if (millis() - _stateStartMs >= NTP_TIMEOUT_MS) {
      Serial.println("[NTP] timeout — no response");
      WiFi.disconnect(true);
      _onFailure();
    }
  }

  void _onSuccess() {
    _failures = 0;
    _nextSyncMs = millis() + NTP_SYNC_INTERVAL_MS;
    _state = SYNC_IDLE;
  }

  void _onFailure() {
    _failures++;
    // Backoff: 1m, 2m, 4m, 8m, … capped at NTP_SYNC_INTERVAL_MS.
    unsigned int shift = (_failures - 1 < 6) ? (_failures - 1) : 6;
    unsigned long retry = kRetryBaseMs << shift;
    if (retry > NTP_SYNC_INTERVAL_MS) retry = NTP_SYNC_INTERVAL_MS;
    _nextSyncMs = millis() + retry;
    _state = SYNC_IDLE;
    Serial.printf("[NTP] failed (#%d), retry in %lus\n",
                  _failures, retry / 1000);
  }

  bool _seedSystemTimeFromRtc() {
    auto dt = M5.Rtc.getDateTime();
    if (dt.date.year < 2025) return false;
    struct tm t = {};
    t.tm_year = dt.date.year - 1900;
    t.tm_mon = dt.date.month - 1;
    t.tm_mday = dt.date.date;
    t.tm_hour = dt.time.hours;
    t.tm_min = dt.time.minutes;
    t.tm_sec = dt.time.seconds;
    // TZ=UTC0 was set in begin(), so mktime treats t as UTC.
    time_t epoch = mktime(&t);
    if (epoch < kMinValidEpoch) return false;
    struct timeval tv = { epoch, 0 };
    settimeofday(&tv, nullptr);
    return true;
  }

  void _writeRtc(const struct tm& utc) {
    m5::rtc_datetime_t dt = {};
    dt.date.year    = utc.tm_year + 1900;
    dt.date.month   = utc.tm_mon + 1;
    dt.date.date    = utc.tm_mday;
    dt.date.weekDay = utc.tm_wday;
    dt.time.hours   = utc.tm_hour;
    dt.time.minutes = utc.tm_min;
    dt.time.seconds = utc.tm_sec;
    M5.Rtc.setDateTime(dt);
  }
};
