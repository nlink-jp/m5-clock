// ntp_sync.h — WiFi connection and NTP time sync
#pragma once

#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include "config.h"
#include "config_manager.h"

class NtpSync {
public:
  NtpSync() : _lastSyncMs(0), _syncing(false) {
    strlcpy(_lastSyncStr, "--:--", sizeof(_lastSyncStr));
  }

  // Initial sync at startup (blocking)
  void beginSync(const ClockConfig& cfg) {
    _syncing = true;
    if (_connectWiFi(cfg)) {
      _syncNtp(cfg);
      WiFi.disconnect(true);
    }
    _syncing = false;
    _lastSyncMs = millis();
  }

  // Periodic sync check (call from loop)
  void update(const ClockConfig& cfg) {
    if (millis() - _lastSyncMs >= NTP_SYNC_INTERVAL_MS) {
      _syncing = true;
      if (_connectWiFi(cfg)) {
        _syncNtp(cfg);
        WiFi.disconnect(true);
      }
      _syncing = false;
      _lastSyncMs = millis();
    }
  }

  // Manual sync trigger
  void manualSync(const ClockConfig& cfg) {
    _syncing = true;
    if (_connectWiFi(cfg)) {
      _syncNtp(cfg);
      WiFi.disconnect(true);
    }
    _syncing = false;
    _lastSyncMs = millis();
  }

  bool isSyncing() const { return _syncing; }
  const char* lastSyncTime() const { return _lastSyncStr; }

private:
  unsigned long _lastSyncMs;
  bool _syncing;
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
    Serial.println("[NTP] syncing...");
    configTime(cfg.gmt_offset_sec, cfg.daylight_offset_sec, cfg.ntp_server);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, NTP_TIMEOUT_MS)) {
      Serial.println("[NTP] failed to get time");
      return false;
    }

    // Update RTC via M5Unified
    auto dt = m5::rtc_datetime_t{{
      static_cast<int16_t>(timeinfo.tm_year + 1900),
      static_cast<int8_t>(timeinfo.tm_mon + 1),
      static_cast<int8_t>(timeinfo.tm_mday)
    }, {
      static_cast<int8_t>(timeinfo.tm_hour),
      static_cast<int8_t>(timeinfo.tm_min),
      static_cast<int8_t>(timeinfo.tm_sec)
    }};
    M5.Rtc.setDateTime(dt);

    snprintf(_lastSyncStr, sizeof(_lastSyncStr), "%02d:%02d",
             timeinfo.tm_hour, timeinfo.tm_min);

    Serial.printf("[NTP] synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                  timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return true;
  }
};
