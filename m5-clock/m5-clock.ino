// m5-clock.ino — NTP-synchronized clock for M5Stack Core2
//
// Time is stored as UTC end-to-end (system clock and RTC). Timezone is
// applied only at display time. NTP sync runs as a non-blocking state
// machine in loop(), so the clock keeps ticking during sync. See
// ntp_sync.h for the full convention.

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
  m5cfg.serial_baudrate = 115200;
  M5.begin(m5cfg);

  Serial.println("=== M5 Clock ===");

  // Load config (works without SD card — falls back to defaults)
  ConfigManager::load(cfg);

  // Initialize display
  display.begin();

  // Seed system clock from RTC if available, schedule first NTP sync.
  // No blocking sync here: if RTC is invalid, the clock view shows a
  // placeholder until the state machine completes its first sync.
  ntp.begin(cfg);

  // Initial brightness from current local hour, defaulting to day if no time.
  struct tm tinfo;
  int hour = ntp.getLocalTime(tinfo) ? tinfo.tm_hour : 12;
  bool night = cfg.night_mode_enabled &&
    ((cfg.night_start_hour > cfg.night_end_hour)
      ? (hour >= cfg.night_start_hour || hour < cfg.night_end_hour)
      : (hour >= cfg.night_start_hour && hour < cfg.night_end_hour));
  M5.Display.setBrightness(night ? cfg.night_brightness : cfg.day_brightness);

  Serial.println("[Main] setup complete");
}

void loop() {
  M5.update();
  unsigned long now = millis();

  // Always step the sync state machine, including while the config view
  // is up. update() returns immediately each iteration.
  ntp.update(cfg);

  if (currentMode == MODE_CLOCK) {
    // Button A: show config
    if (M5.BtnA.wasPressed()) {
      currentMode = MODE_CONFIG;
      configModeMs = now;
      display.drawConfig(cfg);
    }

    // Button C: schedule an immediate NTP sync (non-blocking)
    if (M5.BtnC.wasPressed()) {
      ntp.manualSync(cfg);
    }

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
