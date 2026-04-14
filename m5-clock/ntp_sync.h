// ntp_sync.h — WiFi connection and NTP time sync
// Time is always stored as UTC. Callers apply timezone offset.
#pragma once

#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>
#include "config.h"
#include "config_manager.h"

class NtpSync {
public:
  NtpSync() : _lastSyncMs(0), _syncing(false), _synced(false), _gmtOffset(0) {
    strlcpy(_lastSyncStr, "--:--", sizeof(_lastSyncStr));
  }

  void beginSync(const ClockConfig& cfg) {
    _gmtOffset = cfg.gmt_offset_sec + cfg.daylight_offset_sec;
    _syncing = true;
    if (_connectWiFi(cfg)) {
      _syncNtp(cfg);
      WiFi.disconnect(true);
    }
    _syncing = false;
    _lastSyncMs = millis();
  }

  void update(const ClockConfig& cfg) {
    if (millis() - _lastSyncMs >= NTP_SYNC_INTERVAL_MS) {
      manualSync(cfg);
    }
  }

  void manualSync(const ClockConfig& cfg) {
    _gmtOffset = cfg.gmt_offset_sec + cfg.daylight_offset_sec;
    _syncing = true;
    if (_connectWiFi(cfg)) {
      _syncNtp(cfg);
      WiFi.disconnect(true);
    }
    _syncing = false;
    _lastSyncMs = millis();
  }

  // Get current local time (UTC + offset). Returns false if no time available.
  bool getLocalTime(struct tm& out) const {
    time_t now;
    time(&now);
    // Debug: print once per minute
    static time_t lastDebug = 0;
    if (now - lastDebug >= 60) {
      lastDebug = now;
      struct tm utc;
      gmtime_r(&now, &utc);
      Serial.printf("[Time] epoch=%ld UTC=%02d:%02d:%02d offset=%ld\n",
                    (long)now, utc.tm_hour, utc.tm_min, utc.tm_sec, _gmtOffset);
    }
    if (now < 100000) return false;  // not yet set
    now += _gmtOffset;
    gmtime_r(&now, &out);
    return true;
  }

  bool isSyncing() const { return _syncing; }
  bool hasSynced() const { return _synced; }
  const char* lastSyncTime() const { return _lastSyncStr; }

private:
  unsigned long _lastSyncMs;
  bool _syncing;
  bool _synced;
  long _gmtOffset;
  char _lastSyncStr[6];

  bool _connectWiFi(const ClockConfig& cfg) {
    if (strlen(cfg.ssid) == 0) {
      Serial.println("[NTP] no SSID configured");
      return false;
    }

    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    delay(100);

    if (cfg.static_ip_enabled) {
      IPAddress ip, gw, sn, dns;
      if (ip.fromString(cfg.ip) && gw.fromString(cfg.gateway) &&
          sn.fromString(cfg.subnet) && dns.fromString(cfg.dns1)) {
        WiFi.config(ip, gw, sn, dns);
      }
    }

    Serial.printf("[NTP] connecting to %s\n", cfg.ssid);
    WiFi.begin(cfg.ssid, cfg.password);

    for (int i = 0; i < WIFI_MAX_ATTEMPTS; i++) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[NTP] connected: %s\n", WiFi.localIP().toString().c_str());
        return true;
      }
      delay(WIFI_ATTEMPT_DELAY_MS);
    }

    Serial.println("[NTP] WiFi connection failed");
    WiFi.disconnect(true);
    return false;
  }

  bool _syncNtp(const ClockConfig& cfg) {
    Serial.println("[NTP] syncing (UTC)...");

    // Reset ESP32 system time to force fresh NTP fetch.
    // This clears any stale offset from previous configTime calls.
    struct timeval tv = {0, 0};
    settimeofday(&tv, nullptr);

    // Request UTC only — no timezone offset
    configTime(0, 0, cfg.ntp_server);

    // Wait for NTP to actually update the system clock
    struct tm timeinfo;
    if (!::getLocalTime(&timeinfo, NTP_TIMEOUT_MS)) {
      Serial.println("[NTP] failed to get time");
      return false;
    }

    // Verify the time makes sense (year >= 2025)
    if (timeinfo.tm_year + 1900 < 2025) {
      Serial.println("[NTP] got stale time, retrying...");
      delay(1000);
      if (!::getLocalTime(&timeinfo, NTP_TIMEOUT_MS) || timeinfo.tm_year + 1900 < 2025) {
        Serial.println("[NTP] still stale, giving up");
        return false;
      }
    }

    _synced = true;

    Serial.printf("[NTP] UTC: %04d-%02d-%02d %02d:%02d:%02d\n",
                  timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    // Compute local time for sync indicator display
    time_t utc = mktime(&timeinfo);
    time_t local = utc + _gmtOffset;
    struct tm lt;
    gmtime_r(&local, &lt);
    snprintf(_lastSyncStr, sizeof(_lastSyncStr), "%02d:%02d", lt.tm_hour, lt.tm_min);

    Serial.printf("[NTP] local: %04d-%02d-%02d %02d:%02d (UTC%+ld)\n",
                  lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
                  lt.tm_hour, lt.tm_min, _gmtOffset / 3600);
    return true;
  }
};
