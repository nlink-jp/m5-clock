// config.h — Build-time constants
#pragma once

// NTP sync interval (1 hour)
#define NTP_SYNC_INTERVAL_MS 3600000UL

// Config display timeout
#define CONFIG_DISPLAY_TIMEOUT_MS 10000UL

// Clock update interval
#define CLOCK_UPDATE_INTERVAL_MS 1000

// WiFi connection
#define WIFI_MAX_ATTEMPTS 20
#define WIFI_ATTEMPT_DELAY_MS 500

// NTP timeout
#define NTP_TIMEOUT_MS 5000

// Battery voltage range (Core2 LiPo)
#define BAT_VOLTAGE_MIN 3.5f
#define BAT_VOLTAGE_MAX 4.2f

// SD card config file path
#define CONFIG_FILE_PATH "/config.json"
