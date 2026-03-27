#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

#define TOUCH_PIN T0
#define OUTPUT_PIN 5

int threshold = 50;
// ===== 共有データ =====
volatile int count = 0;
bool wasTouched = false;
String lastServerDate = "";

// ===== 送信用タスク =====
void wifiTask(void *pvParameters) {
  while (true) {
    if (WiFi.status() == WL_CONNECTED) {
      // HTTPClient http;
      HTTPClient http;

      http.begin(SERVER_URL);
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      http.addHeader("Content-Type", "application/json");

      // JSONデータ（ここが変更ポイント）
      String jsonData = "{\"user_id\":\"esp32\",\"action\":\"click\",\"count\":" + String(count) + "}";
      Serial.println(jsonData);
      int httpResponseCode = http.POST(jsonData);

      Serial.print("POST: ");
      Serial.println(httpResponseCode);

      if (httpResponseCode > 0) {
        String responseBody = http.getString();
        Serial.println(responseBody);

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, responseBody);
        if (!err && doc["server_date"].is<const char*>()) {
          String serverDate = String(doc["server_date"].as<const char*>());
          if (lastServerDate.length() > 0 && serverDate != lastServerDate) {
            count = 0;
            Serial.println("Date changed. Count reset to 0.");
          }
          lastServerDate = serverDate;
        }
      }

      http.end();
    } else {
      // Wi-Fi未接続時は再接続を試みる（loop側のタッチ判定は継続）
      WiFi.reconnect();
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS); // 10秒ごと
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(OUTPUT_PIN, OUTPUT);

  // Wi-Fi接続開始（接続完了待ちはしない）
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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
  digitalWrite(OUTPUT_PIN, isTouched ? HIGH : LOW);
  wasTouched = isTouched;
  delay(10); // 軽い安定化
}