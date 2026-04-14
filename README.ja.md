# m5-clock

M5Stack Core2 用 NTP 同期デジタル時計。

日付、曜日、時刻（秒付き）を表示。起動時および1時間ごとにWiFi/NTPで時刻同期し、WiFi不通時はRTCでバックアップ。自動輝度調整（ナイトモード）とSDカード設定に対応。

## ハードウェア

- **M5Stack Core2**（ESP32、LCD 320x240、RTC、PSRAM、バッテリ）

## 機能

- 日付（YYYY-MM-DD）、曜日、時刻（HH:MM:SS）表示
- NTP時刻同期（起動時 + 1時間間隔）
- WiFi不通時のRTCバックアップ
- ナイトモード（時間帯による自動輝度調整）
- バッテリ残量表示
- 静的IP / DHCP 切替
- SDカードJSON設定
- ボタンA: 設定表示、ボタンC: 手動NTP同期

## セットアップ

### Arduino IDE

1. ボードインストール: Board Managerから **M5Stack**（v3.x）
2. ライブラリインストール:
   - `M5Unified`
   - `ArduinoJson`
3. `m5-clock/m5-clock.ino` を開く
4. ボード選択: **M5Stack-Core2**
5. アップロード

### SDカード設定

`config.json.example` を SDカードに `/config.json` としてコピーし編集：

```json
{
  "wifi": {
    "ssid": "your_ssid",
    "password": "your_password"
  },
  "ntp": {
    "server": "pool.ntp.org",
    "timezone": 9,
    "daylight_offset_sec": 0
  },
  "static_ip": {
    "enabled": false,
    "ip": "192.168.1.123",
    "gateway": "192.168.1.1",
    "subnet": "255.255.255.0",
    "dns1": "8.8.8.8"
  },
  "night_mode": {
    "enabled": true,
    "start_hour": 22,
    "end_hour": 7,
    "brightness": 30,
    "day_brightness": 200
  }
}
```

SDカードなしでもデフォルト設定（UTC+9、pool.ntp.org）で動作する。

## アーキテクチャ

```
m5-clock/
├── m5-clock.ino         ← メインスケッチ（setup/loop）
├── config.h             ← ビルド時定数
├── config_manager.h     ← SDカードJSON設定読み取り
├── ntp_sync.h           ← WiFi接続 + NTP同期
└── display_manager.h    ← 時計/設定表示（スプライトダブルバッファ）
```

## v1.x からの移行

v2.0 でレガシー `M5Core2.h` から `M5Unified` に移行：
- ESP32 ボードパッケージ v3.x が必要
- レガシー `M5Core2` ライブラリがインストール済みなら削除
- ライブラリマネージャから `M5Unified` をインストール

## ライセンス

[LICENSE](LICENSE)を参照。
