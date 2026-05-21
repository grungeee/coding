#define PWMpin 15
#define integratedLED 2


void setup() {
  pinMode(PWMpin, OUTPUT);
  pinMode(integratedLED, OUTPUT);
}

void loop() {

  digitalWrite(PWMpin, LOW ? HIGH : LOW);
  digitalWrite(integratedLED, LOW ? HIGH : LOW);
  delay(1000);
  // digitalWrite(PWMpin, HIGH);
  // digitalWrite(integratedLED, HIGH);
}
