#include <M5Cardputer.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

constexpr uint16_t STATUS_INTERVAL_MS = 1000;
constexpr uint16_t CHANNEL_HOP_INTERVAL_MS = 250;
constexpr uint16_t PACKET_BUFFER_SIZE = 240;
constexpr uint8_t FIRST_ESPNOW_CHANNEL = 1;
constexpr uint8_t LAST_ESPNOW_CHANNEL = 13;

char latestMessage[PACKET_BUFFER_SIZE] = {};
uint8_t latestMac[6] = {};
int latestRssi = 0;
int latestLength = 0;
volatile bool packetPending = false;
bool channelLocked = false;
uint8_t currentChannel = 6;
uint32_t packetCount = 0;
uint32_t lastPacketMs = 0;
uint32_t lastStatusMs = 0;
uint32_t lastChannelHopMs = 0;

String macToString(const uint8_t *mac) {
  char text[18];
  snprintf(text, sizeof(text), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(text);
}

void drawHeader() {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.setTextColor(GREEN, BLACK);
  M5Cardputer.Display.println("ESP-NOW RX auto");
  M5Cardputer.Display.setTextColor(WHITE, BLACK);
  M5Cardputer.Display.print("MAC ");
  M5Cardputer.Display.println(WiFi.macAddress());
  M5Cardputer.Display.printf("Scanning ch %u...\n", currentChannel);
  M5Cardputer.Display.println();
}

void setReceiveChannel(uint8_t channel) {
  currentChannel = channel;
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
}

void printWrapped(const char *text) {
  const uint16_t maxCharsPerLine = 39;
  uint16_t column = 0;

  for (const char *p = text; *p != '\0'; ++p) {
    if (*p == '\n' || column >= maxCharsPerLine) {
      M5Cardputer.Display.println();
      column = 0;
      if (*p == '\n') {
        continue;
      }
    }

    M5Cardputer.Display.print(*p);
    ++column;
  }

  M5Cardputer.Display.println();
}

void showPacket() {
  ++packetCount;
  lastPacketMs = millis();

  Serial.print("#");
  Serial.print(packetCount);
  Serial.print(" from ");
  Serial.print(macToString(latestMac));
  Serial.print(" len=");
  Serial.print(latestLength);
  Serial.print(" rssi=");
  Serial.print(latestRssi);
  Serial.print(" msg=");
  Serial.println(latestMessage);

  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.setTextColor(GREEN, BLACK);
  M5Cardputer.Display.printf("Packet #%lu\n", static_cast<unsigned long>(packetCount));
  M5Cardputer.Display.setTextColor(WHITE, BLACK);
  M5Cardputer.Display.printf("Channel %u\n", currentChannel);
  M5Cardputer.Display.print("From ");
  M5Cardputer.Display.println(macToString(latestMac));
  M5Cardputer.Display.printf("Len %d  RSSI %d\n\n", latestLength, latestRssi);
  M5Cardputer.Display.setTextColor(CYAN, BLACK);
  printWrapped(latestMessage);
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  const int copyLength = min(len, static_cast<int>(sizeof(latestMessage) - 1));

  memcpy(latestMessage, data, copyLength);
  latestMessage[copyLength] = '\0';
  memcpy(latestMac, info->src_addr, sizeof(latestMac));
  latestLength = len;
  latestRssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
  channelLocked = true;
  packetPending = true;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  M5Cardputer.begin();
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(WHITE, BLACK);
  drawHeader();

  WiFi.mode(WIFI_STA);
  while (!WiFi.STA.started()) {
    delay(10);
  }
  esp_wifi_set_ps(WIFI_PS_NONE);
  setReceiveChannel(currentChannel);

  Serial.println();
  Serial.println("BOOT: Cardputer ESP-NOW receiver");
  Serial.print("MAC=");
  Serial.println(WiFi.macAddress());
  Serial.println("channel=auto scan 1-13");

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    M5Cardputer.Display.setTextColor(RED, BLACK);
    M5Cardputer.Display.println("ESP-NOW init failed");
    while (true) {
      delay(1000);
    }
  }

  esp_now_register_recv_cb(onDataRecv);
  Serial.println("waiting for ESP-NOW packets");
}

void loop() {
  M5Cardputer.update();

  if (packetPending) {
    packetPending = false;
    showPacket();
  }

  if (!channelLocked && millis() - lastChannelHopMs >= CHANNEL_HOP_INTERVAL_MS) {
    lastChannelHopMs = millis();
    const uint8_t nextChannel = currentChannel >= LAST_ESPNOW_CHANNEL
                                    ? FIRST_ESPNOW_CHANNEL
                                    : currentChannel + 1;
    setReceiveChannel(nextChannel);

    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(GREEN, BLACK);
    M5Cardputer.Display.println("ESP-NOW RX auto");
    M5Cardputer.Display.setTextColor(WHITE, BLACK);
    M5Cardputer.Display.printf("Scanning ch %u...\n", currentChannel);
    M5Cardputer.Display.print("MAC ");
    M5Cardputer.Display.println(WiFi.macAddress());
  }

  if (millis() - lastStatusMs >= STATUS_INTERVAL_MS) {
    lastStatusMs = millis();

    if (lastPacketMs == 0) {
      Serial.print("scanning channel ");
      Serial.println(currentChannel);
    }
  }
}
