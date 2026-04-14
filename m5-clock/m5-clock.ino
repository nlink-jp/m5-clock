// m5-clock.ino — NTP-synchronized clock for M5Stack Core2
//
// Displays date, weekday, and time with NTP sync and RTC backup.
// Features night mode, battery display, and SD card configuration.
// Uses M5Unified for cross-device compatibility.

#include <M5Unified.h>
#include "config.h"
#include "config_manager.h"
#include "ntp_sync.h"
#include "display_manager.h"

// --- Globals ---
ClockConfig cfg;
NtpSync ntp;
DisplayManager display;

enum DisplayMode { MODE_CLOCK, MODE_CONFIG };
DisplayMode currentMode = MODE_CLOCK;
unsigned long configModeMs = 0;
unsigned long lastClockUpdateMs = 0;

void setup() {
  auto m5cfg = M5.config();
  M5.begin(m5cfg);

  Serial.println("=== M5 Clock ===");

  // Load config (works without SD card — falls back to defaults)
  ConfigManager::load(cfg);

  // Initialize display
  display.begin();

  // Initial NTP sync
  M5.Display.setTextSize(2);
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.println("Syncing time...");

  ntp.beginSync(cfg);

  // Set initial brightness
  auto dt = M5.Rtc.getDateTime();
  bool night = cfg.night_mode_enabled &&
    ((cfg.night_start_hour > cfg.night_end_hour)
      ? (dt.time.hours >= cfg.night_start_hour || dt.time.hours < cfg.night_end_hour)
      : (dt.time.hours >= cfg.night_start_hour && dt.time.hours < cfg.night_end_hour));
  M5.Display.setBrightness(night ? cfg.night_brightness : cfg.day_brightness);

  Serial.println("[Main] setup complete");
}

void loop() {
  M5.update();
  unsigned long now = millis();

  if (currentMode == MODE_CLOCK) {
    // Button A: show config
    if (M5.BtnA.wasPressed()) {
      currentMode = MODE_CONFIG;
      configModeMs = now;
      display.drawConfig(cfg);
    }

    // Button C: manual NTP sync
    if (M5.BtnC.wasPressed()) {
      ntp.manualSync(cfg);
    }

    // Periodic NTP sync
    ntp.update(cfg);

    // Update clock display every second
    if (now - lastClockUpdateMs >= CLOCK_UPDATE_INTERVAL_MS) {
      lastClockUpdateMs = now;
      display.drawClock(cfg, ntp);
    }

  } else {
    // Any button or timeout → back to clock
    if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed() ||
        now - configModeMs >= CONFIG_DISPLAY_TIMEOUT_MS) {
      currentMode = MODE_CLOCK;
    }
  }

  delay(50);
}
