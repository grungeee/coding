const int ENA = 2;   // to ENA-
const int DIR = 4;   // to DIR-
const int PUL = 6;   // to PUL-

const long MOTOR_STEPS_PER_REV = 200; // typical NEMA17 (1.8°)
const long MICROSTEPS = 16;           // MUST match TB6600 DIP (1/1,1/2,1/4,1/8,1/16,1/32...)
const long STEPS_PER_REV = MOTOR_STEPS_PER_REV * MICROSTEPS;

const int PULSE_US = 5;               // >= 2.2us, 5us is safe
const int PERIOD_US = 800;            // speed (smaller = faster)

void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(PUL, OUTPUT);

  // common-anode setup: ENA- LOW usually enables
  digitalWrite(ENA, LOW);

  digitalWrite(PUL, HIGH);   // idle
  digitalWrite(DIR, HIGH);   // start CW
}

inline void stepOnce() {
  digitalWrite(PUL, LOW);
  delayMicroseconds(PULSE_US);
  digitalWrite(PUL, HIGH);
}

void loop() {
  static long steps = 0;

  stepOnce();
  steps++;

  delayMicroseconds(PERIOD_US - PULSE_US);

  if (steps >= STEPS_PER_REV) {
    steps = 0;
    digitalWrite(DIR, !digitalRead(DIR)); // flip direction after 1 rev
    delay(200);
  }
}

