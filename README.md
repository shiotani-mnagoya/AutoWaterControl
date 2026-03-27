# AutoWaterControl
水の自動制御装置のプログラムのリポジトリです。

## 概要
このリポジトリは、ESP32で取得したタッチイベントをFastAPIサーバへ送信し、CSVに記録する構成です。

- ESP32側: `src/main.cpp`
- サーバ側: `server/app/main.py`
- 受信データ保存先: `server/data/events.csv`

ESP32は10秒ごとにJSONをPOSTし、サーバ応答の`server_date`を受け取ります。
ESP32はこの`server_date`を監視し、日付が変わったタイミングで`count`を`0`へリセットします。

## ディレクトリ構成（主要）
- `src/main.cpp`: ESP32ファームウェア本体
- `include/secrets.h`: Wi-Fi情報とサーバURL（ローカル専用、コミットしない）
- `include/secrets.example.h`: `secrets.h`のテンプレート
- `server/app/main.py`: FastAPIアプリ
- `server/data/events.csv`: 受信ログCSV
- `server/requirements.txt`: サーバ依存パッケージ

## server/app/main.py 詳細
`server/app/main.py`は、ESP32からのイベントを受け取りCSVへ追記するFastAPIアプリです。

### 実装ポイント
1. FastAPIアプリ初期化
- `app = FastAPI(title="AutoWaterControl API")`

2. CSV出力先とヘッダ定義
- `CSV_PATH = BASE_DIR / "data" / "events.csv"`
- ヘッダ順は次の通りです。
	- `received_at`
	- `server_date`
	- `user_id`
	- `action`
	- `count`
	- `client_ip`

3. リクエストバリデーション（Pydantic）
- `IngestPayload`モデルで受信JSONを検証します。
	- `user_id: str`
	- `action: str`
	- `count: int = Field(ge=0)`（0以上を保証）

4. CSV追記処理
- `append_csv_row()`で以下を実施します。
	- `server/data`ディレクトリを自動作成
	- CSV未作成時はヘッダを書き込み
	- 1リクエストにつき1行追記

5. エンドポイント
- `GET /health`
	- 動作確認用。`{"ok": true}`を返します。
- `POST /ingest`
	- 受信データをCSVへ追記
	- サーバ時刻を生成
		- `server_datetime`: ISO 8601（秒精度、タイムゾーン付き）
		- `server_date`: `YYYY-MM-DD`
	- クライアントIP（`request.client.host`）も記録
	- 応答:
		- `ok`
		- `server_datetime`
		- `server_date`

### POST /ingest の想定JSON
```json
{
	"user_id": "esp32",
	"action": "click",
	"count": 5
}
```

### レスポンス例
```json
{
	"ok": true,
	"server_datetime": "2026-03-26T10:15:30+09:00",
	"server_date": "2026-03-26"
}
```

## src/main.cpp 詳細
`src/main.cpp`は、タッチ入力のカウントと定期HTTP送信を行うESP32コードです。

### 実装ポイント
1. 使用ライブラリ
- `WiFi.h`: Wi-Fi接続
- `HTTPClient.h`: HTTP POST
- `ArduinoJson.h`: サーバ応答JSON解析

2. ハードウェア定義と状態
- `TOUCH_PIN`は`T0`
- `OUTPUT_PIN`は`5`
- `threshold = 50`（タッチ判定閾値）
- 主要変数
	- `volatile int count = 0`: タッチ回数
	- `bool wasTouched = false`: エッジ検出用
	- `String lastServerDate = ""`: 前回受信した日付

3. `setup()`
- シリアル開始（`115200`）
- OUTPUTピンを出力設定
- `WiFi.setAutoReconnect(true)`を有効化して`WiFi.begin(WIFI_SSID, WIFI_PASSWORD)`で接続開始
- 接続完了待ちは行わず、`xTaskCreatePinnedToCore()`で`wifiTask`をCore1へ起動
- これによりWi-Fi未接続でも`loop()`は動き、タッチ判定とOUTPUT_PIN制御は継続

4. `loop()`（高速ポーリング）
- `touchRead(TOUCH_PIN)`で値取得
- `value < threshold`をタッチ中と判定
- エッジ検出（押下開始時のみ）で`count++`
- タッチ中はOUTPUT_PIN HIGH、非タッチ時はLOW
- `delay(10)`で軽い安定化

5. `wifiTask()`（10秒ごとの送信タスク）
- Wi-Fi接続中のみ送信
- Wi-Fi未接続時は`WiFi.reconnect()`を試行
- `SERVER_URL`へ`Content-Type: application/json`でPOST
- 送信JSON:
	- `user_id: "esp32"`
	- `action: "click"`
	- `count: 現在値`
- 成功時にレスポンス本文をJSONとして解析
- `server_date`が取得できた場合:
	- すでに`lastServerDate`があり、かつ新しい日付と異なるとき`count = 0`
	- その後`lastServerDate`を更新
- タスク周期は`vTaskDelay(10000 / portTICK_PERIOD_MS)`（10秒）

### 日付変更時リセットの挙動
- 初回応答: `lastServerDate`のみ設定（リセットなし）
- 2回目以降: `server_date`が変化していれば`count`を0へリセット
- これにより、ESP32側RTCに依存せずサーバ基準で日次カウントを管理できます。

## セットアップ
### 1. ESP32側
1. `include/secrets.example.h`を参考に`include/secrets.h`を作成
2. `WIFI_SSID`, `WIFI_PASSWORD`, `SERVER_URL`を設定

例:
```cpp
#pragma once

#define WIFI_SSID "your-ssid"
#define WIFI_PASSWORD "your-password"
#define SERVER_URL "http://192.168.1.10:8000/ingest"
```

### 2. FastAPI側
1. `server`ディレクトリへ移動
2. 依存パッケージをインストール
3. サーバ起動

```powershell
cd server
pip install -r requirements.txt
uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload
```

または`server/run_server.bat`を利用してください。

## 動作確認
1. `GET /health`へアクセスして`{"ok": true}`を確認
2. ESP32起動後、10秒ごとに`POST /ingest`が呼ばれることを確認
3. `server/data/events.csv`に行が追記されることを確認
4. 日付が変わると、次回応答受信時にESP32の`count`が0へ戻ることを確認

## 注意事項
- `include/secrets.h`は機密情報を含むためコミットしないでください。
- テンプレートとして`include/secrets.example.h`を維持してください。
