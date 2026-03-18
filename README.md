# m5-clock

M5Stack Core2 用の NTP 時計です。Wi-Fi 経由で時刻を自動同期し、日付・曜日・時刻を表示します。

## 必要なもの

**ハードウェア**
- M5Stack Core2
- microSD カード (FAT32 フォーマット)

**Arduino IDE ライブラリ**
- [M5Core2](https://docs.m5stack.com/en/quick_start/m5core2/arduino)
- [ArduinoJson](https://arduinojson.org/v6/doc/installation/)

**Arduino IDE ボードパッケージ**
- M5Stack (2.x 系)

## セットアップ

1. `config.json.example` をコピーして `config.json` にリネームし、内容を編集する
2. `config.json` を SD カードのルートに置く
3. SD カードを M5Stack Core2 に挿入する
4. Arduino IDE でスケッチを書き込む

## config.json

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

### 設定項目

| キー | デフォルト | 説明 |
|------|-----------|------|
| `wifi.ssid` | — | Wi-Fi の SSID |
| `wifi.password` | — | Wi-Fi のパスワード |
| `ntp.server` | `pool.ntp.org` | NTP サーバーのホスト名 |
| `ntp.timezone` | `9` | UTC からの時差（日本は 9）|
| `ntp.daylight_offset_sec` | `0` | サマータイムのオフセット（秒）|
| `static_ip.enabled` | `false` | 静的 IP を使う場合は `true` |
| `static_ip.ip` | — | 端末の IP アドレス |
| `static_ip.gateway` | — | デフォルトゲートウェイ |
| `static_ip.subnet` | — | サブネットマスク |
| `static_ip.dns1` | — | DNS サーバー |
| `night_mode.enabled` | `true` | ナイトモードを有効にする |
| `night_mode.start_hour` | `22` | ナイトモード開始時刻（時）|
| `night_mode.end_hour` | `7` | ナイトモード終了時刻（時）|
| `night_mode.brightness` | `30` | ナイトモード時の輝度 (0–255) |
| `night_mode.day_brightness` | `200` | 通常時の輝度 (0–255) |

各セクションのキーを省略した場合はデフォルト値が使われます。

## 動作

- **起動時**: Wi-Fi 接続を試み、成功したら NTP で時刻を同期して RTC を更新します。Wi-Fi に繋がらない場合は RTC の値でそのまま動作します。
- **1時間ごと**: Wi-Fi に接続して NTP 同期を行い、完了後 Wi-Fi を切断します。
- **ナイトモード**: 指定した時間帯は LCD の輝度を下げ、文字色を赤に切り替えます。日をまたぐ範囲（例: 22時〜7時）にも対応しています。

## 画面表示

```
┌─────────────────────────┐
│                         │
│    2026-03-19           │
│         WED             │
│                         │
│     12:34:56            │
│                         │
│                         │
│ BAT  73%     NTP 12:30  │
└─────────────────────────┘
```

- 左下: バッテリー残量
- 右下: 最終 NTP 同期時刻（同期中は `NTP syncing...` を表示）

## ボタン操作

| ボタン | 機能 |
|--------|------|
| 左 (A) | 設定画面を表示（10秒で自動的に閉じる） |
| 中 (B) | なし |
| 右 (C) | NTP 同期を手動実行 |

設定画面ではいずれかのボタンを押しても閉じられます。Wi-Fi パスワードは表示されません。

## ライセンス

MIT License — © 2026 magifd2
