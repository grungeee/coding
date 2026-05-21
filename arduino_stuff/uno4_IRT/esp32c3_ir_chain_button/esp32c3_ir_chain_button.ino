#include <Arduino.h>
#include <IRremote.hpp>

namespace {
constexpr uint8_t kIrLedPin = 4;
constexpr uint8_t kButtonPin = 3;
constexpr uint8_t kStatusLedPin = 8;

constexpr uint16_t kNecAddress = 0x0001;
constexpr uint16_t kCmdTurnOn = 0x0000;
constexpr uint16_t kCmdTimeDown = 0x0004;
constexpr uint16_t kCmdStart = 0x0001;
constexpr uint16_t kCmdSpeedUp = 0x0002;

constexpr uint8_t kButtonActive = LOW;
constexpr uint32_t kDebounceMs = 40;
constexpr uint16_t kInterCommandDelayMs = 35;
constexpr uint16_t kStepRepeats = 0;

bool lastRawButtonState = HIGH;
bool debouncedButtonState = HIGH;
uint32_t lastDebounceTime = 0;

void setStatusLed(bool enabled) {
  digitalWrite(kStatusLedPin, enabled ? HIGH : LOW);
}

void sendCommand(uint16_t command, uint8_t count, const char *label) {
  for (uint8_t i = 0; i < count; ++i) {
    setStatusLed(true);
    IrSender.sendNEC(kNecAddress, command, kStepRepeats);
    setStatusLed(false);

    Serial.print(label);
    Serial.print(" ");
    Serial.print(i + 1);
    Serial.print("/");
    Serial.println(count);

    delay(kInterCommandDelayMs);
  }
}

void sendSequence() {
  Serial.println("Sending sequence...");
  sendCommand(kCmdTurnOn, 1, "Turn On");
  sendCommand(kCmdTimeDown, 4, "Time Down");
  sendCommand(kCmdStart, 1, "Start");
  sendCommand(kCmdSpeedUp, 29, "Speed Up");
  Serial.println("Sequence done.");
}
}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(kButtonPin, INPUT_PULLUP);
  pinMode(kStatusLedPin, OUTPUT);
  setStatusLed(false);
  IrSender.begin(kIrLedPin);

  Serial.println();
  Serial.println("ESP32-C3 IR chain sender ready.");
  Serial.println("KY-005 signal pin: GPIO4");
  Serial.println("Button pin: GPIO3 to GND");
  Serial.println("Status LED pin: GPIO8");
}

void loop() {
  const bool rawState = digitalRead(kButtonPin);

  if (rawState != lastRawButtonState) {
    lastDebounceTime = millis();
    lastRawButtonState = rawState;
  }

  if ((millis() - lastDebounceTime) > kDebounceMs && rawState != debouncedButtonState) {
    debouncedButtonState = rawState;

    if (debouncedButtonState == kButtonActive) {
      sendSequence();

      while (digitalRead(kButtonPin) == kButtonActive) {
        delay(10);
      }
    }
  }
}
