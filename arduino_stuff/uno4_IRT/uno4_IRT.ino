#include <Arduino.h>
#include <ArduinoGraphics.h>
#include <Arduino_LED_Matrix.h>

#define DISABLE_CODE_FOR_RECEIVER
#include <IRremote.hpp>

namespace {

constexpr uint8_t IR_LED_PIN = 3;
constexpr uint8_t NEC_ADDRESS = 0x01;
constexpr uint8_t FIRST_COMMAND = 0x00;
constexpr uint8_t LAST_COMMAND = 0x07;
constexpr uint8_t IR_REPEATS = 4;
constexpr uint8_t BURSTS_PER_COMMAND = 4;
constexpr unsigned long BURST_GAP_MS = 160;
constexpr unsigned long COMMAND_HOLD_MS = 2200;

ArduinoLEDMatrix matrix;

void showText(const char *text, uint8_t speed = 65) {
  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(speed);
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.print("   ");
  matrix.println(text);
  matrix.print("   ");
  matrix.endText(SCROLL_LEFT);
  matrix.endDraw();
}

void showCommand(uint8_t command) {
  char label[5];
  snprintf(label, sizeof(label), "%X", command);
  showText(label, 45);
}

void flashBuiltinLed() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(35);
  digitalWrite(LED_BUILTIN, LOW);
}

void sendNecCommand(uint8_t command) {
  Serial.print(F("NEC addr=0x"));
  Serial.print(NEC_ADDRESS, HEX);
  Serial.print(F(" cmd=0x"));
  Serial.println(command, HEX);

  showCommand(command);

  for (uint8_t burst = 0; burst < BURSTS_PER_COMMAND; ++burst) {
    flashBuiltinLed();
    IrSender.sendNEC(NEC_ADDRESS, command, IR_REPEATS);

    if (burst + 1 < BURSTS_PER_COMMAND) {
      delay(BURST_GAP_MS);
    }
  }

  delay(COMMAND_HOLD_MS);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  matrix.begin();
  showText("NEC");

  IrSender.begin(IR_LED_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);

  Serial.println(F("UNO R4 WiFi NEC command cycler"));
  Serial.print(F("IR sender pin: D"));
  Serial.println(IR_LED_PIN);
  Serial.print(F("NEC address: 0x"));
  Serial.println(NEC_ADDRESS, HEX);
  Serial.println(F("Cycling commands 0x0 to 0x7."));
}

void loop() {
  for (uint8_t command = FIRST_COMMAND; command <= LAST_COMMAND; ++command) {
    sendNecCommand(command);
  }
}
