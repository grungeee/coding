// Wiring (common-cathode, Uno 5V):
// TB6600 PUL-/DIR-/ENA- -> GND
// Uno 4 -> PUL+
// Uno 3 -> DIR+
// Uno 2 -> ENA+
// Uno 13 -> NC toggle button to GND (press opens, triggers move/reverse)
// Uno 8 -> relay module signal 0 (active-low: starts LOW; goes HIGH on press, back LOW after 0.5s)
// Uno 9 -> relay module signal 1 (active-low: pulls LOW on press)
// Uno 10 -> relay module signal 2 (active-low: pulls LOW on press)
// Tested step periods (worked on this setup): 5000, 3000, 2000, 1500, 1200, 1000, 800, 600 µs

const int STEP_PIN = 4; // PUL+
const int DIR_PIN  = 3; // DIR+
const int EN_PIN   = 2; // ENA+
const int STOP_PIN = 13; // NC button to GND (INPUT_PULLUP)

const int RELAY_PIN_0 = 8; // relay control
const int RELAY_PIN_1 = 9; // relay control
const int RELAY_PIN_2 = 10; // relay control

const long MOTOR_STEPS_PER_REV = 200; // ST2818L1404-A is 1.8°
const long MICROSTEPS = 1;            // TB6600 DIP: ON ON OFF = full step (200 pulses/rev)
const long STEPS_PER_REV = MOTOR_STEPS_PER_REV * MICROSTEPS;

const unsigned int PULSE_US  = 5;     // >2.2us is usually required
// Timing: constant speed (no acceleration/deceleration).
const unsigned int RUN_PERIOD_US   = 6000; // constant speed (smaller = faster)

// Motion target: move a 27 mm wheel forward by 688 mm.
const float WHEEL_DIAMETER_MM = 29.0f;
// const float TRAVEL_MM = 688.0f;
// Steps needed ≈ 1,622 (~8.11 revs) with full steps.
// const long TARGET_STEPS = (long)((TRAVEL_MM / (PI * WHEEL_DIAMETER_MM)) * STEPS_PER_REV + 0.5f);
// const long TARGET_STEPS = 1800; // 688+19mm (27mm)
const long TARGET_STEPS = 1850; // 688mm

bool dirState = false; // tracked so we can flip without rewiring
bool moving = false;   // set true to run the move
int lastButtonState = HIGH;
unsigned long buttonPressMs = 0;
bool relay0Pending = false;
bool waitForRelay0 = false; // block motion until relay0 reactivates
bool relays12Active = false;

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  pinMode(STOP_PIN, INPUT_PULLUP); // NC -> GND; reads HIGH when opened/pressed
  pinMode(RELAY_PIN_0, OUTPUT);
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  digitalWrite(RELAY_PIN_0, LOW);  // default: relay0 active (active-low)
  digitalWrite(RELAY_PIN_1, HIGH); // relay1 inactive
  digitalWrite(RELAY_PIN_2, HIGH); // relay2 inactive

  lastButtonState = digitalRead(STOP_PIN);

  digitalWrite(DIR_PIN, dirState ? HIGH : LOW); // start direction

  // TB6600 ENA is often active-low. Try LOW first; if that disables, switch to HIGH.
  digitalWrite(EN_PIN, LOW); // drive ENA+ low to enable (common-cathode wiring)

  digitalWrite(STEP_PIN, LOW);  // idle
}

inline void stepOnce() {
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(PULSE_US);
  digitalWrite(STEP_PIN, LOW);
}

void flipDir() {
  dirState = !dirState;
  digitalWrite(DIR_PIN, dirState ? HIGH : LOW);
}

void loop() {
  static long stepsTaken = 0;

  // Edge-detect the NC button (LOW normally, HIGH when pressed/opened).
  int buttonState = digitalRead(STOP_PIN);

  // Press edge: flip direction, restart move, drop relay0 for 500ms, activate relay1/2.
  if (buttonState == HIGH && lastButtonState == LOW) {
    flipDir();       // reverse direction each press
    stepsTaken = 0;  // restart move
    moving = false;  // wait until relay0 is back on
    waitForRelay0 = true;
    // Relay sequencing (active-low): deactivate relay0, activate relay1/2, re-activate relay0 after 0.5s.
    digitalWrite(RELAY_PIN_0, HIGH); // off
    digitalWrite(RELAY_PIN_1, LOW);  // on
    digitalWrite(RELAY_PIN_2, LOW);  // on
    buttonPressMs = millis();
    relay0Pending = true;
    relays12Active = true;
  }

  // Release edge: stop motion and drop relay0 again for 500ms; turn off relay1/2.
  if (buttonState == LOW && lastButtonState == HIGH) {
    moving = false;
    waitForRelay0 = true;
    digitalWrite(RELAY_PIN_0, HIGH); // off
    digitalWrite(RELAY_PIN_1, HIGH); // off
    digitalWrite(RELAY_PIN_2, HIGH); // off
    buttonPressMs = millis();
    relay0Pending = true;
    relays12Active = false;
  }
  lastButtonState = buttonState;

  // Re-enable relay0 after 500 ms from press.
  if (relay0Pending && (millis() - buttonPressMs >= 500)) {
    digitalWrite(RELAY_PIN_0, LOW); // back on (active-low)
    relay0Pending = false;
    if (waitForRelay0) {
      moving = true;      // allow motion after safety delay
      waitForRelay0 = false;
    }
  }

  // When button not pressed and no pending timing, enforce default relay state.
  if (buttonState == LOW && !relay0Pending) {
    digitalWrite(RELAY_PIN_0, LOW);  // default on (active-low)
    digitalWrite(RELAY_PIN_1, relays12Active ? LOW : HIGH);
    digitalWrite(RELAY_PIN_2, relays12Active ? LOW : HIGH);
  }

  if (!moving) {
    return;
  }

  if (stepsTaken >= TARGET_STEPS) {
    // Hold position after reaching the target distance.
    moving = false;
    return;
  }

  // Step toward the target distance.
  stepOnce();
  stepsTaken++;

  // Timing: constant speed only.
  if (RUN_PERIOD_US > PULSE_US) {
    delayMicroseconds(RUN_PERIOD_US - PULSE_US);
  } else {
    delayMicroseconds(1);
  }
}
