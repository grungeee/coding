// Motor A connections
const int motorA1 = 2;  // IN1
const int motorA2 = 3;  // IN2
const int enableA = 9;  // PWM speed control

// Motor B connections
const int motorB1 = 4;  // IN3
const int motorB2 = 5;  // IN4
const int enableB = 10; // PWM speed control

void setup() {
  // Set all motor control pins as outputs
  pinMode(motorA1, OUTPUT);
  pinMode(motorA2, OUTPUT);
  pinMode(enableA, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);
  pinMode(enableB, OUTPUT);
}

void loop() {
  // Example motor control sequence
  
  // Run both motors forward at full speed
  runMotor(1, 255, true);
  runMotor(2, 255, true);
  delay(2000);
  
  // Stop both motors
  stopMotor(1);
  stopMotor(2);
  delay(1000);
  
  // Run motor A forward and motor B backward at half speed
  runMotor(1, 128, true);
  runMotor(2, 128, false);
  delay(2000);
  
  // Stop both motors
  stopMotor(1);
  stopMotor(2);
  delay(1000);
}

void runMotor(int motor, int speed, bool forward) {
  if (motor == 1) {
    digitalWrite(motorA1, forward ? HIGH : LOW);
    digitalWrite(motorA2, forward ? LOW : HIGH);
    analogWrite(enableA, speed);
  } else if (motor == 2) {
    digitalWrite(motorB1, forward ? HIGH : LOW);
    digitalWrite(motorB2, forward ? LOW : HIGH);
    analogWrite(enableB, speed);
  }
}

void stopMotor(int motor) {
  if (motor == 1) {
    digitalWrite(motorA1, LOW);
    digitalWrite(motorA2, LOW);
    analogWrite(enableA, 0);
  } else if (motor == 2) {
    digitalWrite(motorB1, LOW);
    digitalWrite(motorB2, LOW);
    analogWrite(enableB, 0);
  }
}
