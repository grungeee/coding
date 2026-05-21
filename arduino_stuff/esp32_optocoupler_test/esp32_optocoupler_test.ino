const int OUTPUT_PIN = 15;
const int LED_PIN = 2;

const unsigned long HALF_PERIOD_MS = 500; // 1 Hz blink: 500 ms on, 500 ms off.

void setup() {
  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(OUTPUT_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  digitalWrite(OUTPUT_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  delay(HALF_PERIOD_MS);

  digitalWrite(OUTPUT_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  delay(HALF_PERIOD_MS);
}
