/*
  Classic ESP32 wireless controller.

  Reads:
    pot1   -> GPIO32
    pot2   -> GPIO34
    pot3   -> GPIO35
    switch -> GPIO33, closes to GND

  Sends the values with ESP-NOW on channel 6.
  Prints pot and switch values to Serial at 115200 baud.
*/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

const int pot1Pin = 32;
const int pot2Pin = 34;
const int pot3Pin = 35;
const int switchPin = 33;

const uint8_t espNowChannel = 6;
const uint16_t payloadMagic = 0xC3A5;
const uint8_t receiverAddress[] = {0x08, 0xa6, 0xf7, 0xa8, 0xcb, 0x18};

struct PotPacket {
  uint16_t magic;
  uint16_t sequence;
  uint16_t pot1;
  uint16_t pot2;
  uint16_t pot3;
  uint8_t switchOn;
};

uint16_t sequence = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("BOOT: Classic ESP32 wireless controller");

  pinMode(switchPin, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  while (!WiFi.STA.started()) {
    delay(10);
  }
  esp_wifi_set_channel(espNowChannel, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    ESP.restart();
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = espNowChannel;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    ESP.restart();
  }
}

void loop() {
  PotPacket packet;
  packet.magic = payloadMagic;
  packet.sequence = sequence++;
  packet.pot1 = analogRead(pot1Pin);
  packet.pot2 = analogRead(pot2Pin);
  packet.pot3 = analogRead(pot3Pin);
  packet.switchOn = digitalRead(switchPin) == LOW;

  esp_now_send(receiverAddress, (uint8_t *)&packet, sizeof(packet));

  Serial.print("seq=");
  Serial.print(packet.sequence);
  Serial.print(" pot1=");
  Serial.print(packet.pot1);
  Serial.print(" pot2=");
  Serial.print(packet.pot2);
  Serial.print(" pot3=");
  Serial.print(packet.pot3);
  Serial.print(" switch=");
  Serial.println(packet.switchOn ? "ON" : "OFF");

  delay(20);
}
