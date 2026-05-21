#include <WiFi.h>
#include <esp_now.h>
#include <M5Cardputer.h>

struct SensorPacket {
  char senderName[16];
  uint32_t readingId;
  float temperatureC;
  float humidityPct;
};

constexpr uint32_t STALE_AFTER_MS = 10000;

SensorPacket latestPacket = {};
char latestMacAddress[18] = "--:--:--:--:--:--";
volatile bool hasPacket = false;
volatile bool packetUpdated = false;
volatile uint32_t lastPacketMillis = 0;

portMUX_TYPE packetMux = portMUX_INITIALIZER_UNLOCKED;

String deviceMac;
uint32_t lastStatusDrawMillis = 0;

void drawHeader() {
  M5Cardputer.Display.fillRect(0, 0, 240, 28, TFT_NAVY);
  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_NAVY);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(8, 8);
  M5Cardputer.Display.print("ESP-NOW DHT11 Receiver");
}

void drawBootScreen(const char *status) {
  M5Cardputer.Display.fillScreen(TFT_BLACK);
  drawHeader();

  M5Cardputer.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  M5Cardputer.Display.setCursor(8, 42);
  M5Cardputer.Display.print(status);

  M5Cardputer.Display.setCursor(8, 62);
  M5Cardputer.Display.print("MAC:");
  M5Cardputer.Display.setCursor(8, 76);
  M5Cardputer.Display.print(deviceMac);

  M5Cardputer.Display.setCursor(8, 110);
  M5Cardputer.Display.print("Waiting for sensor packet...");
}

void drawWaitingScreen(bool stale) {
  M5Cardputer.Display.fillScreen(TFT_BLACK);
  drawHeader();

  M5Cardputer.Display.setTextColor(stale ? TFT_ORANGE : TFT_LIGHTGREY, TFT_BLACK);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setCursor(8, 46);
  M5Cardputer.Display.print(stale ? "Signal stale" : "Waiting");

  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  M5Cardputer.Display.setCursor(8, 82);
  M5Cardputer.Display.print(stale ? "No packet for 10+ seconds" : "Listening for ESP-NOW data");

  M5Cardputer.Display.setCursor(8, 112);
  M5Cardputer.Display.print("This MAC: ");
  M5Cardputer.Display.print(deviceMac);
}

void drawPacketScreen(const SensorPacket &packet, const char *macAddress, uint32_t ageMs) {
  const bool stale = ageMs > STALE_AFTER_MS;
  M5Cardputer.Display.fillScreen(TFT_BLACK);
  drawHeader();

  M5Cardputer.Display.setTextColor(stale ? TFT_ORANGE : TFT_GREEN, TFT_BLACK);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(8, 34);
  M5Cardputer.Display.print(stale ? "STALE" : "LIVE");

  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Cardputer.Display.setTextSize(3);
  M5Cardputer.Display.setCursor(8, 50);
  M5Cardputer.Display.print(packet.temperatureC, 1);
  M5Cardputer.Display.print(" C");

  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setCursor(8, 86);
  M5Cardputer.Display.print(packet.humidityPct, 1);
  M5Cardputer.Display.print(" %RH");

  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  M5Cardputer.Display.setCursor(8, 112);
  M5Cardputer.Display.print(packet.senderName);
  M5Cardputer.Display.print(" #");
  M5Cardputer.Display.print(packet.readingId);

  M5Cardputer.Display.setCursor(130, 112);
  M5Cardputer.Display.print(ageMs / 1000);
  M5Cardputer.Display.print("s ago");

  M5Cardputer.Display.setCursor(8, 124);
  M5Cardputer.Display.print(macAddress);
}

void onDataReceive(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len) {
  if (len != sizeof(SensorPacket)) {
    Serial.print("Unexpected packet size: ");
    Serial.println(len);
    return;
  }

  SensorPacket packet;
  memcpy(&packet, incomingData, sizeof(packet));
  packet.senderName[sizeof(packet.senderName) - 1] = '\0';

  char macAddress[18];
  snprintf(
    macAddress,
    sizeof(macAddress),
    "%02X:%02X:%02X:%02X:%02X:%02X",
    recvInfo->src_addr[0],
    recvInfo->src_addr[1],
    recvInfo->src_addr[2],
    recvInfo->src_addr[3],
    recvInfo->src_addr[4],
    recvInfo->src_addr[5]
  );

  portENTER_CRITICAL_ISR(&packetMux);
  latestPacket = packet;
  memcpy(latestMacAddress, macAddress, sizeof(latestMacAddress));
  lastPacketMillis = millis();
  hasPacket = true;
  packetUpdated = true;
  portEXIT_CRITICAL_ISR(&packetMux);

  Serial.print("From ");
  Serial.print(packet.senderName);
  Serial.print(" (");
  Serial.print(macAddress);
  Serial.print(") | Reading #");
  Serial.print(packet.readingId);
  Serial.print(" | Temperature: ");
  Serial.print(packet.temperatureC, 1);
  Serial.print(" C | Humidity: ");
  Serial.print(packet.humidityPct, 1);
  Serial.println(" %");
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  deviceMac = WiFi.macAddress();

  Serial.print("This Cardputer MAC: ");
  Serial.println(deviceMac);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    drawBootScreen("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onDataReceive);
  Serial.println("ESP-NOW receiver ready");
  drawBootScreen("ESP-NOW receiver ready");
}

void setup() {
  Serial.begin(115200);
  delay(300);

  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setBrightness(180);

  Serial.println("Starting Cardputer ESP-NOW receiver");
  setupEspNow();
}

void loop() {
  M5Cardputer.update();

  const uint32_t now = millis();
  bool shouldRedraw = false;
  bool localHasPacket = false;
  SensorPacket localPacket = {};
  char localMacAddress[18] = "--:--:--:--:--:--";
  uint32_t localLastPacketMillis = 0;

  portENTER_CRITICAL(&packetMux);
  if (packetUpdated) {
    packetUpdated = false;
    shouldRedraw = true;
  }
  localHasPacket = hasPacket;
  localPacket = latestPacket;
  memcpy(localMacAddress, latestMacAddress, sizeof(localMacAddress));
  localLastPacketMillis = lastPacketMillis;
  portEXIT_CRITICAL(&packetMux);

  if (now - lastStatusDrawMillis >= 1000) {
    shouldRedraw = true;
  }

  if (shouldRedraw) {
    lastStatusDrawMillis = now;

    if (localHasPacket) {
      drawPacketScreen(localPacket, localMacAddress, now - localLastPacketMillis);
    } else {
      drawWaitingScreen(false);
    }
  }

  delay(20);
}
