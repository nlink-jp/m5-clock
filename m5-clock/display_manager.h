// display_manager.h — Clock and config display for M5Stack Core2
// Core2 has PSRAM → sprite double-buffering is safe.
//
// Time source: NtpSync.getLocalTime() is the single authority. RTC is
// already read by NtpSync.begin() to seed the system clock, so this layer
// never needs to fall back to a raw RTC read with its own offset logic.
#pragma once

#include <M5Unified.h>
#include <time.h>
#include "config.h"
#include "config_manager.h"
#include "ntp_sync.h"

static const char* WEEK_DAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

class DisplayManager {
public:
  DisplayManager() : _sprite(&M5.Display) {}

  void begin() {
    _sprite.createSprite(M5.Display.width(), M5.Display.height());
  }

  void drawClock(const ClockConfig& cfg, NtpSync& ntp) {
    struct tm timeinfo;
    bool haveTime = ntp.getLocalTime(timeinfo);

    int hour = haveTime ? timeinfo.tm_hour : 0;
    bool night = haveTime && _isNightMode(cfg, hour);
    _applyBrightness(cfg, night);

    uint16_t textColor = night ? RED : WHITE;
    uint16_t dimColor  = night ? 0xC000 : TFT_DARKGREY;

    _sprite.fillSprite(BLACK);
    _sprite.setTextColor(textColor, BLACK);

    if (haveTime) {
      // Date (YYYY-MM-DD)
      _sprite.setTextSize(3);
      _sprite.setCursor(40, 40);
      _sprite.printf("%04d-%02d-%02d",
                     timeinfo.tm_year + 1900,
                     timeinfo.tm_mon + 1,
                     timeinfo.tm_mday);

      // Weekday
      _sprite.setCursor(120, 80);
      _sprite.print(WEEK_DAYS[timeinfo.tm_wday % 7]);

      // Time (HH:MM:SS)
      _sprite.setTextSize(5);
      _sprite.setCursor(50, 130);
      _sprite.printf("%02d:%02d:%02d",
                     timeinfo.tm_hour,
                     timeinfo.tm_min,
                     timeinfo.tm_sec);
    } else {
      // No valid time source yet — show placeholder.
      _sprite.setTextSize(3);
      _sprite.setCursor(40, 40);
      _sprite.print("----------");
      _sprite.setCursor(120, 80);
      _sprite.print("---");
      _sprite.setTextSize(5);
      _sprite.setCursor(50, 130);
      _sprite.print("--:--:--");
    }

    // Footer
    _sprite.setTextSize(1);
    _sprite.setTextColor(dimColor, BLACK);

    int batPct = _batteryPercent();
    _sprite.setCursor(4, 224);
    _sprite.printf("BAT %3d%%", batPct);

    if (ntp.isSyncing()) {
      _sprite.setCursor(220, 224);
      _sprite.print("NTP syncing..");
    } else {
      _sprite.setCursor(220, 224);
      _sprite.printf("NTP %-5s", ntp.lastSyncTime());
    }

    _sprite.pushSprite(0, 0);
  }

  void drawConfig(const ClockConfig& cfg) {
    _sprite.fillSprite(BLACK);
    _sprite.setTextColor(WHITE, BLACK);
    _sprite.setTextSize(2);
    _sprite.setCursor(0, 0);

    _sprite.println("[ Config ]");
    _sprite.printf("SSID : %s\n", cfg.ssid);
    _sprite.printf("NTP  : %s\n", cfg.ntp_server);
    _sprite.printf("TZ   : UTC+%ld\n", cfg.gmt_offset_sec / 3600);

    if (cfg.static_ip_enabled) {
      _sprite.printf("IP   : %s\n", cfg.ip);
      _sprite.printf("GW   : %s\n", cfg.gateway);
    } else {
      _sprite.println("IP   : DHCP");
    }

    if (cfg.night_mode_enabled) {
      _sprite.printf("Night: %d:00 - %d:00\n",
                      cfg.night_start_hour, cfg.night_end_hour);
    } else {
      _sprite.println("Night: OFF");
    }

    _sprite.setTextSize(1);
    _sprite.setTextColor(TFT_DARKGREY, BLACK);
    _sprite.setCursor(55, 224);
    _sprite.print("Press any button to close");

    _sprite.pushSprite(0, 0);
  }

private:
  M5Canvas _sprite;
  bool _prevNight = false;

  bool _isNightMode(const ClockConfig& cfg, int hour) {
    if (!cfg.night_mode_enabled) return false;
    if (cfg.night_start_hour > cfg.night_end_hour) {
      return hour >= cfg.night_start_hour || hour < cfg.night_end_hour;
    }
    return hour >= cfg.night_start_hour && hour < cfg.night_end_hour;
  }

  void _applyBrightness(const ClockConfig& cfg, bool night) {
    if (night != _prevNight) {
      M5.Display.setBrightness(night ? cfg.night_brightness : cfg.day_brightness);
      _prevNight = night;
    }
  }

  int _batteryPercent() {
    int32_t level = M5.Power.getBatteryLevel();
    if (level >= 0) return level;
    return 0;
  }
};
