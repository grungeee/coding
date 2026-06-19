#include <Arduino.h>
#include <IRremote.hpp>
#include <esp32-hal-bt.h>
#include <esp_sleep.h>
#include <esp_wifi.h>

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

void setStatusLed(bool enabled) {
  digitalWrite(kStatusLedPin, enabled ? HIGH : LOW);
}

void disableRadios() {
  esp_wifi_stop();
  esp_wifi_deinit();

#if SOC_BT_SUPPORTED
  btStop();
#endif
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

bool buttonIsPressed() {
  return digitalRead(kButtonPin) == kButtonActive;
}

bool buttonIsPressedAfterDebounce() {
  if (!buttonIsPressed()) {
    return false;
  }

  delay(kDebounceMs);
  return buttonIsPressed();
}

void waitForButtonRelease() {
  while (buttonIsPressed()) {
    delay(10);
  }

  delay(kDebounceMs);
}

void enterDeepSleep() {
  setStatusLed(false);
  pinMode(kIrLedPin, OUTPUT);
  digitalWrite(kIrLedPin, LOW);

  disableRadios();

  const uint64_t wakePinMask = 1ULL << kButtonPin;
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_deep_sleep_enable_gpio_wakeup(wakePinMask, ESP_GPIO_WAKEUP_GPIO_LOW);
#if SOC_PM_SUPPORT_MODEM_PD
  esp_sleep_pd_config(ESP_PD_DOMAIN_MODEM, ESP_PD_OPTION_OFF);
#endif

  Serial.println("Entering deep sleep. Press button to wake and send.");
  Serial.flush();
  esp_deep_sleep_start();
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

  const esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();
  if (wakeupCause == ESP_SLEEP_WAKEUP_GPIO || buttonIsPressedAfterDebounce()) {
    sendSequence();
    waitForButtonRelease();
  }

  enterDeepSleep();
}

void loop() {
}
