
#include <WiFi.h>
#include <esp_now.h>

constexpr uint8_t DHT_PIN = 4;
constexpr unsigned long READ_INTERVAL_MS = 2000;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct SensorPacket {
  char senderName[16];
  uint32_t readingId;
  float temperatureC;
  float humidityPct;
};

SensorPacket packet = {"esp32-dht11", 0, 0.0f, 0.0f};

bool waitForLevel(uint8_t expectedLevel, unsigned long timeoutUs) {
  unsigned long start = micros();
  while (digitalRead(DHT_PIN) != expectedLevel) {
    if (micros() - start > timeoutUs) {
      return false;
    }
  }
  return true;
}

bool readDht11(float &temperatureC, float &humidityPct) {
  uint8_t data[5] = {0, 0, 0, 0, 0};

  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delay(20);
  digitalWrite(DHT_PIN, HIGH);
  delayMicroseconds(40);
  pinMode(DHT_PIN, INPUT_PULLUP);

  if (!waitForLevel(LOW, 100)) {
    return false;
  }
  if (!waitForLevel(HIGH, 100)) {
    return false;
  }
  if (!waitForLevel(LOW, 100)) {
    return false;
  }

  noInterrupts();
  for (int byteIndex = 0; byteIndex < 5; ++byteIndex) {
    for (int bitIndex = 0; bitIndex < 8; ++bitIndex) {
      if (!waitForLevel(HIGH, 70)) {
        interrupts();
        return false;
      }

      unsigned long highStart = micros();
      if (!waitForLevel(LOW, 120)) {
        interrupts();
        return false;
      }

      data[byteIndex] <<= 1;
      if (micros() - highStart > 40) {
        data[byteIndex] |= 1;
      }
    }
  }
  interrupts();

  uint8_t checksum = data[0] + data[1] + data[2] + data[3];
  if (checksum != data[4]) {
    return false;
  }

  humidityPct = data[0];
  temperatureC = data[2];
  return true;
}

void onDataSent(const uint8_t *macAddr, esp_now_send_status_t status) {
  Serial.print("ESP-NOW send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "success" : "failed");
}

bool initEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return false;
  }

  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(broadcastAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add broadcast peer");
      return false;
    }
  }

  return true;
}

void printLocalReading(const SensorPacket &reading) {
  Serial.print("Reading #");
  Serial.print(reading.readingId);
  Serial.print(" | Temperature: ");
  Serial.print(reading.temperatureC, 1);
  Serial.print(" C | Humidity: ");
  Serial.print(reading.humidityPct, 1);
  Serial.println(" %");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(DHT_PIN, INPUT_PULLUP);

  Serial.println("Starting DHT11 ESP-NOW broadcaster");
  Serial.print("This ESP32 MAC: ");
  Serial.println(WiFi.macAddress());

  if (!initEspNow()) {
    Serial.println("ESP-NOW setup failed. Restarting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }
}

void loop() {
  static unsigned long lastReadMs = 0;
  if (millis() - lastReadMs < READ_INTERVAL_MS) {
    return;
  }
  lastReadMs = millis();

  float humidity = 0.0f;
  float temperature = 0.0f;
  if (!readDht11(temperature, humidity)) {
    Serial.println("Failed to read from DHT11 sensor");
    return;
  }

  packet.readingId++;
  packet.temperatureC = temperature;
  packet.humidityPct = humidity;

  printLocalReading(packet);

  esp_err_t result = esp_now_send(
    broadcastAddress,
    reinterpret_cast<const uint8_t *>(&packet),
    sizeof(packet)
  );

  if (result != ESP_OK) {
    Serial.print("ESP-NOW send error: ");
    Serial.println(result);
  }
}
