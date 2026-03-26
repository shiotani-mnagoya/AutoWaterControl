// #include <Arduino.h>
// int touchPin = T0;   // タッチセンサーピン
// int ledPin = 5;      // LED接続ピン

// int threshold = 60;  // 閾値（調整必要）

// void setup() {
//   Serial.begin(115200);
//   pinMode(ledPin, OUTPUT);
// }

// void loop() {
//   int value = touchRead(touchPin);
//   Serial.println(value);

//   if (value < threshold) {
//     digitalWrite(ledPin, HIGH);  // タッチで点灯
//   } else {
//     digitalWrite(ledPin, LOW);   // 離すと消灯
//   }

//   delay(50);
// }
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "DESKTOP-TU5CKDL 2048";
const char* password = "qC66,917";

String serverUrl = "https://script.google.com/macros/s/AKfycbwv2hPk6ybB5bYykPPKfHXGZeF1CQssD9y_49MqtogPjYPfsEYfiGesXNLB0aLKfO6Jkw/exec";

#define TOUCH_PIN T0
#define LED_PIN 5

int threshold = 50;
// ===== 共有データ =====
volatile int count = 0;
bool wasTouched = false;

// ===== 送信用タスク =====
void wifiTask(void *pvParameters) {
  while (true) {
    if (WiFi.status() == WL_CONNECTED) {
      // HTTPClient http;
      HTTPClient http;

      http.begin(serverUrl);
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      http.addHeader("Content-Type", "application/json");

      // JSONデータ（ここが変更ポイント）
      String jsonData = "{\"user_id\":\"esp32\",\"action\":\"click\",\"count\":" + String(count) + "}";
      Serial.println(jsonData);
      int httpResponseCode = http.POST(jsonData);

      Serial.print("POST: ");
      Serial.println(httpResponseCode);

      http.end();
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS); // 10秒ごと
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Wi-Fi接続
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  // ===== タスク作成 =====
  xTaskCreatePinnedToCore(
    wifiTask,     // 関数
    "wifiTask",   // 名前
    8192,         // スタックサイズ
    NULL,
    1,            // 優先度
    NULL,
    1             // Core1で実行
  );
}

void loop() {
  int value = touchRead(TOUCH_PIN);
  bool isTouched = (value < threshold);

  // エッジ検出（押した瞬間だけカウント）
  if (isTouched && !wasTouched) {
    count++;
    Serial.println("Count: " + String(count));
  }

  // LED
  digitalWrite(LED_PIN, isTouched ? HIGH : LOW);
  wasTouched = isTouched;
  delay(10); // 軽い安定化
}