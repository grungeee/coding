/*
  Classic ESP32 ESP-NOW receiver and PWM output board.

  Receives pot values from the wireless controller and outputs:
    pwm1 -> GPIO5
    pwm2 -> GPIO18
    pwm3 -> GPIO19

  Switch-controlled digital outputs:
    out1 -> GPIO4
    out2 -> GPIO16
    out3 -> GPIO17
*/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

const int pwm1Pin = 5;
const int pwm2Pin = 18;
const int pwm3Pin = 19;
const int switchOut1Pin = 4;
const int switchOut2Pin = 16;
const int switchOut3Pin = 17;

const uint8_t espNowChannel = 6;
const uint16_t payloadMagic = 0xC3A5;
const uint8_t controllerAddress[] = {0xa4, 0xcb, 0x8f, 0x22, 0x52, 0x74};
const int pwmFreq = 15000;
const int pwmResolution = 12;
const int adcMax = 4095;
const float pwmFullScaleVoltage = 3.3f;
const float switchOutputHighVoltage = 3.3f;
const unsigned long signalTimeoutMs = 1000;
const unsigned long statusIntervalMs = 500;

struct PotPacket {
  uint16_t magic;
  uint16_t sequence;
  uint16_t pot1;
  uint16_t pot2;
  uint16_t pot3;
  uint8_t switchOn;
};

volatile bool packetPending = false;
PotPacket latestPacket = {};
unsigned long lastPacketMs = 0;
unsigned long lastStatusMs = 0;

float pwmVoltage(uint16_t value) {
  return (value * pwmFullScaleVoltage) / adcMax;
}

float switchOutputVoltage(bool switchOn) {
  return switchOn ? switchOutputHighVoltage : 0.0f;
}

void applyOutputs(const PotPacket &packet) {
  ledcWrite(pwm1Pin, packet.pot1);
  ledcWrite(pwm2Pin, packet.pot2);
  ledcWrite(pwm3Pin, packet.pot3);

  int switchOutputState = packet.switchOn ? HIGH : LOW;
  digitalWrite(switchOut1Pin, switchOutputState);
  digitalWrite(switchOut2Pin, switchOutputState);
  digitalWrite(switchOut3Pin, switchOutputState);
}

void stopOutputs() {
  ledcWrite(pwm1Pin, 0);
  ledcWrite(pwm2Pin, 0);
  ledcWrite(pwm3Pin, 0);
  digitalWrite(switchOut1Pin, LOW);
  digitalWrite(switchOut2Pin, LOW);
  digitalWrite(switchOut3Pin, LOW);
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(PotPacket)) {
    return;
  }

  PotPacket packet;
  memcpy(&packet, data, sizeof(packet));
  if (packet.magic != payloadMagic) {
    return;
  }

  latestPacket = packet;
  packetPending = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("BOOT: ESP32 ESP-NOW PWM receiver");

  Serial.println("BOOT: attach PWM");
  ledcAttach(pwm1Pin, pwmFreq, pwmResolution);
  ledcAttach(pwm2Pin, pwmFreq, pwmResolution);
  ledcAttach(pwm3Pin, pwmFreq, pwmResolution);

  Serial.println("BOOT: configure switch outputs");
  pinMode(switchOut1Pin, OUTPUT);
  pinMode(switchOut2Pin, OUTPUT);
  pinMode(switchOut3Pin, OUTPUT);
  stopOutputs();

  Serial.println("BOOT: start WiFi STA");
  WiFi.mode(WIFI_STA);
  while (!WiFi.STA.started()) {
    delay(10);
  }
  esp_wifi_set_channel(espNowChannel, WIFI_SECOND_CHAN_NONE);

  Serial.print("MAC=");
  Serial.println(WiFi.macAddress());
  Serial.print("channel=");
  Serial.println(espNowChannel);

  Serial.println("BOOT: init ESP-NOW");
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed; restarting");
    delay(1000);
    ESP.restart();
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, controllerAddress, 6);
  peerInfo.channel = espNowChannel;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("failed to add controller peer");
  }

  esp_now_register_recv_cb(onDataRecv);
  Serial.println("waiting for controller packets");
}

void loop() {
  if (packetPending) {
    packetPending = false;
    PotPacket packet = latestPacket;
    lastPacketMs = millis();
    applyOutputs(packet);

    Serial.print("seq=");
    Serial.print(packet.sequence);
    Serial.print(" pot1=");
    Serial.print(packet.pot1);
    Serial.print(" pwm1_out=");
    Serial.print(pwmVoltage(packet.pot1), 2);
    Serial.print("V pot2=");
    Serial.print(packet.pot2);
    Serial.print(" pwm2_out=");
    Serial.print(pwmVoltage(packet.pot2), 2);
    Serial.print("V pot3=");
    Serial.print(packet.pot3);
    Serial.print(" pwm3_out=");
    Serial.print(pwmVoltage(packet.pot3), 2);
    Serial.print("V switch=");
    Serial.print(packet.switchOn ? "ON" : "OFF");
    Serial.print(" switch_out=");
    Serial.print(switchOutputVoltage(packet.switchOn), 2);
    Serial.print("V outputs=");
    Serial.println(packet.switchOn ? "HIGH" : "LOW");
  }

  if (lastPacketMs != 0 && millis() - lastPacketMs > signalTimeoutMs) {
    stopOutputs();
    lastPacketMs = 0;
    Serial.println("signal timeout, outputs stopped");
  }

  if (lastPacketMs == 0 && millis() - lastStatusMs > statusIntervalMs) {
    lastStatusMs = millis();
    Serial.println("waiting for controller packets");
  }
}
