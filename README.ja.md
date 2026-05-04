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

`sdcard/config.example.json` を SDカードに `/config.json` としてコピーし編集：

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
    "brightness": 60,
    "day_brightness": 200
  }
}
```

| フィールド | デフォルト | 説明 |
|-----------|----------|------|
| `wifi.ssid` | (空) | WiFi SSID |
| `wifi.password` | (空) | WiFiパスワード |
| `ntp.server` | `pool.ntp.org` | NTPサーバー |
| `ntp.timezone` | `9` | UTCオフセット（時間） |
| `ntp.daylight_offset_sec` | `0` | 夏時間オフセット（秒） |
| `static_ip.enabled` | `false` | DHCPの代わりに静的IPを使用 |
| `night_mode.enabled` | `true` | 自動輝度調整を有効化 |
| `night_mode.start_hour` | `22` | ナイトモード開始時刻（24h） |
| `night_mode.end_hour` | `7` | ナイトモード終了時刻（24h） |
| `night_mode.brightness` | `60` | 夜間のLCD輝度（0-255） |
| `night_mode.day_brightness` | `200` | 昼間のLCD輝度（0-255） |

SDカードなしでもデフォルト設定（UTC+9、pool.ntp.org）で動作する。

## アーキテクチャ

```
m5-clock/
├── m5-clock/
│   ├── m5-clock.ino         ← メインスケッチ（setup/loop）
│   ├── config.h             ← ビルド時定数
│   ├── config_manager.h     ← SDカードJSON設定読み取り
│   ├── ntp_sync.h           ← WiFi + NTP同期（内部UTC、表示時にTZ適用）
│   └── display_manager.h    ← 時計/設定表示（スプライトダブルバッファ）
└── sdcard/
    └── config.example.json  ← SDカード設定テンプレート
```

## 技術ノート

- **時刻処理:** システムクロックも RTC も常に UTC で保持し、表示時にのみタイムゾーンオフセットを適用する。TZ 環境変数は起動時に `UTC0` に固定され、`mktime`/`gmtime_r` は常に UTC として解釈される。
- **起動経路:** 起動時に RTC からシステムクロックを seed するため、NTP の応答を待たずに正しい時刻表示が立ち上がる。NTP 同期成功時には RTC に UTC を書き戻すので、電源断・再起動でも時刻が保持される。
- **ノンブロッキング同期:** Wi-Fi 接続から NTP 応答待ちまでを `loop()` 駆動の状態機械として実装しており、同期中も時計表示は 1Hz で更新され続ける（フッターに `NTP syncing..` を表示）。
- **同期リトライ:** NTP 同期失敗時は指数バックオフ（1分・2分・4分…、上限は通常同期間隔）で再試行する。1時間放置されない。
- **ナイトモード:** M5UnifiedはCore2のバックライト輝度を正しく制御する。旧M5Core2.hの`setBrightness()`は正常に動作しなかった。
- **SDカード:** Core2はGPIO4をSD CSピンとして使用。明示的な`SD.begin(4)`が必要。

## v1.x からの移行

v2.0 でレガシー `M5Core2.h` から `M5Unified` に移行：
- ESP32 ボードパッケージ v3.x が必要
- レガシー `M5Core2` ライブラリがインストール済みなら削除
- ライブラリマネージャから `M5Unified` をインストール

## ライセンス

[LICENSE](LICENSE)を参照。
