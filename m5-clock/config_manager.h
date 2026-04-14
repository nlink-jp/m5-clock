// config_manager.h — SD card JSON configuration
#pragma once

#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "config.h"

struct ClockConfig {
  // WiFi
  char ssid[64];
  char password[64];

  // NTP
  char ntp_server[64];
  long gmt_offset_sec;
  int daylight_offset_sec;

  // Static IP
  bool static_ip_enabled;
  char ip[16];
  char gateway[16];
  char subnet[16];
  char dns1[16];

  // Night mode
  bool night_mode_enabled;
  int night_start_hour;
  int night_end_hour;
  int night_brightness;
  int day_brightness;
};

class ConfigManager {
public:
  static bool load(ClockConfig& cfg) {
    // Defaults — clock works even without SD card
    memset(&cfg, 0, sizeof(cfg));
    strlcpy(cfg.ssid, "", sizeof(cfg.ssid));
    strlcpy(cfg.password, "", sizeof(cfg.password));
    strlcpy(cfg.ntp_server, "pool.ntp.org", sizeof(cfg.ntp_server));
    cfg.gmt_offset_sec = 9 * 3600;  // UTC+9 (JST)
    cfg.daylight_offset_sec = 0;
    cfg.static_ip_enabled = false;
    cfg.night_mode_enabled = true;
    cfg.night_start_hour = 22;
    cfg.night_end_hour = 7;
    cfg.night_brightness = 60;
    cfg.day_brightness = 200;

    // Core2 SD card CS pin = GPIO4
    if (!SD.begin(4)) {
      Serial.println("[Config] SD card not found, using defaults");
      return false;
    }

    File file = SD.open(CONFIG_FILE_PATH);
    if (!file) {
      Serial.printf("[Config] %s not found on SD, using defaults\n", CONFIG_FILE_PATH);
      return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) {
      Serial.printf("[Config] JSON parse error: %s\n", err.c_str());
      return false;
    }

    // WiFi
    if (doc["wifi"]["ssid"].is<const char*>())
      strlcpy(cfg.ssid, doc["wifi"]["ssid"], sizeof(cfg.ssid));
    if (doc["wifi"]["password"].is<const char*>())
      strlcpy(cfg.password, doc["wifi"]["password"], sizeof(cfg.password));

    // NTP
    if (doc["ntp"]["server"].is<const char*>())
      strlcpy(cfg.ntp_server, doc["ntp"]["server"], sizeof(cfg.ntp_server));
    cfg.gmt_offset_sec = (doc["ntp"]["timezone"] | 9) * 3600;
    cfg.daylight_offset_sec = doc["ntp"]["daylight_offset_sec"] | 0;

    // Static IP
    cfg.static_ip_enabled = doc["static_ip"]["enabled"] | false;
    if (cfg.static_ip_enabled) {
      strlcpy(cfg.ip, doc["static_ip"]["ip"] | "192.168.1.123", sizeof(cfg.ip));
      strlcpy(cfg.gateway, doc["static_ip"]["gateway"] | "192.168.1.1", sizeof(cfg.gateway));
      strlcpy(cfg.subnet, doc["static_ip"]["subnet"] | "255.255.255.0", sizeof(cfg.subnet));
      strlcpy(cfg.dns1, doc["static_ip"]["dns1"] | "8.8.8.8", sizeof(cfg.dns1));
    }

    // Night mode
    cfg.night_mode_enabled = doc["night_mode"]["enabled"] | true;
    cfg.night_start_hour = doc["night_mode"]["start_hour"] | 22;
    cfg.night_end_hour = doc["night_mode"]["end_hour"] | 7;
    cfg.night_brightness = doc["night_mode"]["brightness"] | 30;
    cfg.day_brightness = doc["night_mode"]["day_brightness"] | 200;

    Serial.printf("[Config] loaded: SSID=%s NTP=%s TZ=UTC+%ld\n",
                  cfg.ssid, cfg.ntp_server, cfg.gmt_offset_sec / 3600);
    return true;
  }
};
