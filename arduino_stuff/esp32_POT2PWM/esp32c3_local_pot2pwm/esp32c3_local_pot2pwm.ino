/*
  Standalone ESP32-C3 potentiometer to PWM converter.

  Local analog inputs:
    pot1 -> GPIO0
    pot2 -> GPIO1
    pot3 -> GPIO3
    switch -> GPIO4, closes to GND

  Local PWM outputs:
    pwm1 -> GPIO5
    pwm2 -> GPIO6
    pwm3 -> GPIO7

  Switch-controlled digital outputs:
    out1 -> GPIO8
    out2 -> GPIO9
    out3 -> GPIO10

  No Serial output.
*/

const int pot1Pin = 0;
const int pot2Pin = 1;
const int pot3Pin = 3;
const int switchPin = 4;

const int pwm1Pin = 5;
const int pwm2Pin = 6;
const int pwm3Pin = 7;
const int switchOut1Pin = 8;
const int switchOut2Pin = 9;
const int switchOut3Pin = 10;

const int pwmFreq = 15000;
const int pwmResolution = 12;

void setup() {
  ledcAttach(pwm1Pin, pwmFreq, pwmResolution);
  ledcAttach(pwm2Pin, pwmFreq, pwmResolution);
  ledcAttach(pwm3Pin, pwmFreq, pwmResolution);

  pinMode(switchPin, INPUT_PULLUP);
  pinMode(switchOut1Pin, OUTPUT);
  pinMode(switchOut2Pin, OUTPUT);
  pinMode(switchOut3Pin, OUTPUT);

  digitalWrite(switchOut1Pin, LOW);
  digitalWrite(switchOut2Pin, LOW);
  digitalWrite(switchOut3Pin, LOW);
}

void loop() {
  uint16_t pot1 = analogRead(pot1Pin);
  uint16_t pot2 = analogRead(pot2Pin);
  uint16_t pot3 = analogRead(pot3Pin);
  bool switchOn = digitalRead(switchPin) == LOW;

  ledcWrite(pwm1Pin, pot1);
  ledcWrite(pwm2Pin, pot2);
  ledcWrite(pwm3Pin, pot3);

  int switchState = switchOn ? HIGH : LOW;
  digitalWrite(switchOut1Pin, switchState);
  digitalWrite(switchOut2Pin, switchState);
  digitalWrite(switchOut3Pin, switchState);
}
