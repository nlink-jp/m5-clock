// config.h — Build-time constants
#pragma once

// NTP sync interval (1 hour)
#define NTP_SYNC_INTERVAL_MS 3600000UL

// Config display timeout
#define CONFIG_DISPLAY_TIMEOUT_MS 10000UL

// Clock update interval
#define CLOCK_UPDATE_INTERVAL_MS 1000

// WiFi connection timeout (state-machine sync polls until connected or timeout)
#define WIFI_TIMEOUT_MS 10000UL

// NTP response timeout (after configTime, polled non-blocking)
#define NTP_TIMEOUT_MS 5000UL

// Battery voltage range (Core2 LiPo)
#define BAT_VOLTAGE_MIN 3.5f
#define BAT_VOLTAGE_MAX 4.2f

// SD card config file path
#define CONFIG_FILE_PATH "/config.json"
