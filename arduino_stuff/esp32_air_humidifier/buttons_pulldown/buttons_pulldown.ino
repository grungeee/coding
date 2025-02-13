int BUTTON_0 = 1;
int BUTTON_1 = 2;
int LIGHTING = 6;
int HUMIDIFIER = 7;
bool button_0_state = false;
bool button_1_state = false;

void setup() {

  pinMode(BUTTON_0, INPUT_PULLUP);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(LIGHTING, OUTPUT);
  pinMode(HUMIDIFIER, OUTPUT);

}

void loop() {
  if (digitalRead(BUTTON_0) == LOW) {
    button_0_state = !button_0_state;
    digitalWrite(LIGHTING, button_0_state);
    delay(100);
  }

  if (digitalRead(BUTTON_1) == LOW) {
    digitalWrite(HUMIDIFIER, HIGH);
  } else {
    digitalWrite(HUMIDIFIER, LOW);
  }
}
