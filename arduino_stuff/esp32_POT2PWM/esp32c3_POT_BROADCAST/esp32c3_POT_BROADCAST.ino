// ESP32-S3: print potentiometer analog value to USB Serial.
// Compile/upload with USB CDC On Boot enabled.

#ifndef POT_PIN
#define POT_PIN A5
#endif

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t READ_INTERVAL_MS = 200;

uint32_t lastReadMs = 0;

void setup() {
  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);
  pinMode(POT_PIN, INPUT);

  Serial.begin(SERIAL_BAUD);
}

void loop() {
  const uint32_t now = millis();

  if (now - lastReadMs < READ_INTERVAL_MS) {
    return;
  }

  lastReadMs = now;
  Serial.println(analogRead(POT_PIN));
}
