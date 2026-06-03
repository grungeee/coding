/*
  ESP32-C3 + 3-pin IR proximity sensor + L298N motor controller

  Wiring:
    IR sensor OUT -> GPIO8
    L298N ENA     -> GPIO5
    L298N IN1     -> GPIO6
    L298N IN2     -> GPIO7

  Power notes:
    - Connect ESP32 GND, IR sensor GND, and L298N GND together.
    - Power the motor from the L298N motor supply, not from the ESP32.
*/

const int SENSOR_PIN = 8;

const int L298_ENA_PIN = 5;
const int L298_IN1_PIN = 6;
const int L298_IN2_PIN = 7;

// Most 3-pin LM393-style IR proximity modules pull OUT LOW when something is close.
// Change this to false if your sensor output goes HIGH when it detects an object.
const bool SENSOR_ACTIVE_LOW = true;

void motorOn()
{
  digitalWrite(L298_IN1_PIN, HIGH);
  digitalWrite(L298_IN2_PIN, LOW);
  digitalWrite(L298_ENA_PIN, HIGH);
}

void motorOff()
{
  digitalWrite(L298_ENA_PIN, LOW);
  digitalWrite(L298_IN1_PIN, LOW);
  digitalWrite(L298_IN2_PIN, LOW);
}

bool objectDetected()
{
  const int sensorValue = digitalRead(SENSOR_PIN);
  return SENSOR_ACTIVE_LOW ? sensorValue == LOW : sensorValue == HIGH;
}

void setup()
{
  pinMode(SENSOR_PIN, INPUT_PULLUP);

  pinMode(L298_ENA_PIN, OUTPUT);
  pinMode(L298_IN1_PIN, OUTPUT);
  pinMode(L298_IN2_PIN, OUTPUT);

  motorOff();

  Serial.begin(115200);
  Serial.println("Automatic tape roller ready");
}

void loop()
{
  if (objectDetected()) {
    motorOn();
  } else {
    motorOff();
  }
}
