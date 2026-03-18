// M5Stack Core2 & Arduino IDE
// =================================================================
// 1. Install M5Stack Core2 Library
//    https://docs.m5stack.com/en/quick_start/m5core2/arduino
// 2. Install ArduinoJson Library
//    https://arduinojson.org/v6/doc/installation/

#include <M5Core2.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "time.h"

// 設定情報を保持する構造体
struct Config {
  char ssid[64];
  char password[64];
  char ntp_server[64];
  long gmt_offset_sec;
  int daylight_offset_sec;

  // 静的IPアドレス設定
  bool static_ip_enabled;
  char ip[16];
  char gateway[16];
  char subnet[16];
  char dns1[16];

  // ナイトモード設定
  bool night_mode_enabled;
  int night_start_hour;
  int night_end_hour;
  int night_brightness;
  int day_brightness;
};

Config config;
RTC_TimeTypeDef rtc_time;
RTC_DateTypeDef rtc_date;
const char* week_days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

// ちらつき防止のためのスプライトオブジェクト
TFT_eSprite clk_sprite = TFT_eSprite(&M5.Lcd);

// NTP同期の間隔 (1時間)
const unsigned long NTP_SYNC_INTERVAL = 3600000UL;
unsigned long lastSyncMillis = 0;

// 最終NTP同期時刻 ("HH:MM" or "--:--")
char lastNtpSyncStr[6] = "--:--";
bool syncInProgress = false;

// 表示モード
enum DisplayMode { MODE_CLOCK, MODE_CONFIG };
DisplayMode currentMode = MODE_CLOCK;
unsigned long configModeMillis = 0;
const unsigned long CONFIG_DISPLAY_TIMEOUT = 10000UL;

// SDカードから設定を読み込む関数
bool loadConfig() {
  if (!SD.begin()) {
    Serial.println("Card failed, or not present");
    M5.Lcd.println("SD Card failed or not present.");
    return false;
  }

  File configFile = SD.open("/config.json");
  if (!configFile) {
    Serial.println("Failed to open config file");
    M5.Lcd.println("Failed to open /config.json");
    return false;
  }

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    M5.Lcd.println("Failed to parse config file.");
    return false;
  }

  strlcpy(config.ssid, doc["wifi"]["ssid"] | "default_ssid", sizeof(config.ssid));
  strlcpy(config.password, doc["wifi"]["password"] | "", sizeof(config.password));
  strlcpy(config.ntp_server, doc["ntp"]["server"] | "pool.ntp.org", sizeof(config.ntp_server));
  config.gmt_offset_sec = (doc["ntp"]["timezone"] | 9) * 3600;
  config.daylight_offset_sec = doc["ntp"]["daylight_offset_sec"] | 0;

  config.static_ip_enabled = doc["static_ip"]["enabled"] | false;
  if (config.static_ip_enabled) {
    strlcpy(config.ip, doc["static_ip"]["ip"] | "192.168.1.123", sizeof(config.ip));
    strlcpy(config.gateway, doc["static_ip"]["gateway"] | "192.168.1.1", sizeof(config.gateway));
    strlcpy(config.subnet, doc["static_ip"]["subnet"] | "255.255.255.0", sizeof(config.subnet));
    strlcpy(config.dns1, doc["static_ip"]["dns1"] | "8.8.8.8", sizeof(config.dns1));
  }

  config.night_mode_enabled = doc["night_mode"]["enabled"] | true;
  config.night_start_hour   = doc["night_mode"]["start_hour"] | 22;
  config.night_end_hour     = doc["night_mode"]["end_hour"] | 7;
  config.night_brightness   = doc["night_mode"]["brightness"] | 30;
  config.day_brightness     = doc["night_mode"]["day_brightness"] | 200;

  return true;
}

// Wi-Fiに接続する関数 (サイレント、成否をboolで返す)
bool connectWiFi() {
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  delay(100);

  if (config.static_ip_enabled) {
    IPAddress staticIP, gateway, subnet, dns1;
    if (staticIP.fromString(config.ip) && gateway.fromString(config.gateway) &&
        subnet.fromString(config.subnet) && dns1.fromString(config.dns1)) {
      WiFi.config(staticIP, gateway, subnet, dns1);
      Serial.println("Using Static IP configuration.");
    } else {
      Serial.println("Failed to parse static IP config.");
    }
  } else {
    Serial.println("Using DHCP.");
  }

  Serial.printf("Connecting to SSID: [%s]\n", config.ssid);
  WiFi.begin(config.ssid, config.password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (attempts++ > 20) {
      Serial.printf("\nWiFi connection failed. Status: %d\n", WiFi.status());
      WiFi.disconnect(true);
      return false;
    }
  }

  Serial.println("\nWiFi connected.");
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
  return true;
}

// NTPで時刻を同期し、RTCを更新する関数 (成否をboolで返す)
bool syncNtpTime() {
  Serial.println("Syncing time with NTP...");

  configTime(config.gmt_offset_sec, config.daylight_offset_sec, config.ntp_server);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 5000)) {
    Serial.println("Failed to obtain time.");
    return false;
  }

  rtc_date.Year    = timeinfo.tm_year + 1900;
  rtc_date.Month   = timeinfo.tm_mon + 1;
  rtc_date.Date    = timeinfo.tm_mday;
  rtc_date.WeekDay = (timeinfo.tm_wday == 0) ? 7 : timeinfo.tm_wday;

  rtc_time.Hours   = timeinfo.tm_hour;
  rtc_time.Minutes = timeinfo.tm_min;
  rtc_time.Seconds = timeinfo.tm_sec;

  M5.Rtc.SetTime(&rtc_time);
  M5.Rtc.SetDate(&rtc_date);

  snprintf(lastNtpSyncStr, sizeof(lastNtpSyncStr), "%02d:%02d", rtc_time.Hours, rtc_time.Minutes);

  Serial.println("RTC updated successfully.");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  return true;
}

// 同期中インジケーターをLCDに直接描画する
void drawSyncIndicator(const char* msg) {
  uint16_t color = isNightMode() ? TFT_MAROON : TFT_DARKGREY;
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(color, BLACK);
  M5.Lcd.setCursor(220, 224);
  M5.Lcd.printf("%-12s", msg); // 固定幅で上書き消去
}

// Wi-Fi接続してNTP同期、切断までを行う
void syncTimeViaNtp() {
  syncInProgress = true;
  drawSyncIndicator("NTP syncing...");

  if (connectWiFi()) {
    syncNtpTime();
    WiFi.disconnect(true);
  } else {
    Serial.println("WiFi unavailable, skipping NTP sync. Using RTC.");
  }

  syncInProgress = false;
  lastSyncMillis = millis();
}

// 設定画面を表示する関数
void displayConfig() {
  clk_sprite.fillSprite(BLACK);
  clk_sprite.setTextColor(WHITE, BLACK);
  clk_sprite.setTextSize(2);
  clk_sprite.setCursor(0, 0);

  clk_sprite.println("[ Config ]");
  clk_sprite.printf("SSID : %s\n", config.ssid);
  clk_sprite.printf("NTP  : %s\n", config.ntp_server);
  clk_sprite.printf("TZ   : UTC+%ld\n", config.gmt_offset_sec / 3600);

  if (config.static_ip_enabled) {
    clk_sprite.printf("IP   : %s\n", config.ip);
    clk_sprite.printf("GW   : %s\n", config.gateway);
  } else {
    clk_sprite.println("IP   : DHCP");
  }

  if (config.night_mode_enabled) {
    clk_sprite.printf("Night: %d:00 - %d:00\n", config.night_start_hour, config.night_end_hour);
  } else {
    clk_sprite.println("Night: OFF");
  }

  clk_sprite.setTextSize(1);
  clk_sprite.setTextColor(TFT_DARKGREY, BLACK);
  clk_sprite.setCursor(55, 224);
  clk_sprite.print("Press any button to close");

  clk_sprite.pushSprite(0, 0);
}

// ナイトモードかどうかを判定する関数
bool isNightMode() {
  if (!config.night_mode_enabled) return false;
  int h = rtc_time.Hours;
  if (config.night_start_hour > config.night_end_hour) {
    // 日をまたぐ場合 (例: 22〜7)
    return h >= config.night_start_hour || h < config.night_end_hour;
  } else {
    return h >= config.night_start_hour && h < config.night_end_hour;
  }
}

// 時計を表示する関数 (ちらつき防止版)
void displayClock() {
  M5.Rtc.GetTime(&rtc_time);
  M5.Rtc.GetDate(&rtc_date);

  static bool prevNightMode = false;
  bool nightMode = isNightMode();
  if (nightMode != prevNightMode) {
    M5.Lcd.setBrightness(nightMode ? config.night_brightness : config.day_brightness);
    prevNightMode = nightMode;
  }

  uint16_t textColor = nightMode ? RED : WHITE;

  // スプライトに描画
  clk_sprite.fillSprite(BLACK);
  clk_sprite.setTextColor(textColor, BLACK);

  // 日付 (YYYY-MM-DD)
  clk_sprite.setTextSize(3);
  clk_sprite.setCursor(40, 40);
  clk_sprite.printf("%04d-%02d-%02d", rtc_date.Year, rtc_date.Month, rtc_date.Date);

  // 曜日
  clk_sprite.setCursor(120, 80);
  clk_sprite.print(week_days[rtc_date.WeekDay % 7]);

  // 時刻 (HH:MM:SS)
  clk_sprite.setTextSize(5);
  clk_sprite.setCursor(50, 130);
  clk_sprite.printf("%02d:%02d:%02d", rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);

  uint16_t dimColor = nightMode ? TFT_MAROON : TFT_DARKGREY;
  clk_sprite.setTextSize(1);
  clk_sprite.setTextColor(dimColor, BLACK);

  // バッテリー残量 (小さく左下に)
  float voltage = M5.Axp.GetBatVoltage();
  int batPct = constrain((int)((voltage - 3.5f) / (4.2f - 3.5f) * 100), 0, 100);
  clk_sprite.setCursor(4, 224);
  clk_sprite.printf("BAT %3d%%", batPct);

  // 最終NTP同期時刻 (小さく右下に)
  if (!syncInProgress) {
    clk_sprite.setCursor(220, 224);
    clk_sprite.printf("NTP %-5s", lastNtpSyncStr);
  }

  // 描画内容を画面に一括転送
  clk_sprite.pushSprite(0, 0);
}

void setup() {
  M5.begin();
  Serial.begin(115200);

  // スプライトを作成
  clk_sprite.createSprite(M5.Lcd.width(), M5.Lcd.height());

  M5.Lcd.setTextSize(2);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);

  M5.Lcd.println("Loading configuration...");
  if (!loadConfig()) {
    M5.Lcd.println("Failed to load config.");
    while(1) { delay(100); }
  }
  M5.Lcd.println("Config loaded.");

  M5.Lcd.println("Connecting to WiFi...");
  if (connectWiFi()) {
    M5.Lcd.println("WiFi connected.");
    M5.Lcd.println("Syncing NTP...");
    if (syncNtpTime()) {
      M5.Lcd.println("NTP sync OK.");
    } else {
      M5.Lcd.println("NTP sync failed. Using RTC.");
    }
    WiFi.disconnect(true);
  } else {
    M5.Lcd.println("WiFi failed. Using RTC.");
  }
  lastSyncMillis = millis();

  // 初期輝度を設定
  M5.Rtc.GetTime(&rtc_time);
  M5.Lcd.setBrightness(isNightMode() ? config.night_brightness : config.day_brightness);

  delay(2000);
}

void loop() {
  M5.update();

  if (currentMode == MODE_CLOCK) {
    // 左ボタン: 設定画面を開く
    if (M5.BtnA.wasPressed()) {
      currentMode = MODE_CONFIG;
      configModeMillis = millis();
    }
    // 右ボタン: 手動でNTP同期
    if (M5.BtnC.wasPressed()) {
      syncTimeViaNtp();
    }
    // 定期NTP同期
    if (millis() - lastSyncMillis >= NTP_SYNC_INTERVAL) {
      syncTimeViaNtp();
    }
    // 1秒ごとに時計を更新
    static unsigned long lastClockUpdate = 0;
    if (millis() - lastClockUpdate >= 1000) {
      lastClockUpdate = millis();
      displayClock();
    }

  } else { // MODE_CONFIG
    // いずれかのボタン or タイムアウトで時計画面に戻る
    if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed() ||
        millis() - configModeMillis >= CONFIG_DISPLAY_TIMEOUT) {
      currentMode = MODE_CLOCK;
    } else {
      displayConfig();
    }
  }

  delay(50);
}
