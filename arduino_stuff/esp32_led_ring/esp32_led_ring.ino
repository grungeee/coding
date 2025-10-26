#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

//#define PIN 3 //esp32c3
#define PIN 13 //esp32
#define NUMPIXELS 12
#define LEFT_BUTTON_PIN 20
#define RIGHT_BUTTON_PIN 21

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Variables that correspond to each half of the circle
const int firstHalfStart = 0;
const int firstHalfEnd = NUMPIXELS / 2;
const int secondHalfStart = NUMPIXELS / 2;
const int secondHalfEnd = NUMPIXELS;

void setup() {
  pixels.begin();
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLDOWN);
}

// Function to blink a specific half of the ring once
void blinkHalf(int start, int end, uint32_t color) {
  // Turn on the specified half
  for (int p = start; p < end; p++) {
    pixels.setPixelColor(p, color);
  }
  pixels.show();
  delay(500); // 500ms on

  // Turn off the specified half
  for (int p = start; p < end; p++) {
    pixels.setPixelColor(p, 0);
  }
  pixels.show();
  delay(500); // 500ms off
}

// Function for the faster red dimming sequence in 100 stages
void redDimmingSequence() {
  const int stages = 100;
  const int total_duration_ms = 1000; // 10 seconds total
  int delay_ms = total_duration_ms / stages; // 20ms per stage

  float brightness_step = 255.0 / (stages - 1);

  // Fade from brightest to darkest
  for (int i = 0; i < stages; i++) {
    int brightness = 255 - (i * brightness_step);
    if (brightness < 0) brightness = 0; // Clamp the value

    uint32_t redColor = pixels.Color(brightness, 0, 0);
    for (int p = 0; p < NUMPIXELS; p++) {
      pixels.setPixelColor(p, redColor);
    }
    pixels.show();
    delay(delay_ms);
  }

  // Fade from darkest to brightest
  for (int i = 0; i < stages; i++) {
    int brightness = i * brightness_step;
    if (brightness > 255) brightness = 255; // Clamp the value

    uint32_t redColor = pixels.Color(brightness, 0, 0);
    for (int p = 0; p < NUMPIXELS; p++) {
      pixels.setPixelColor(p, redColor);
    }
    pixels.show();
    delay(delay_ms);
  }
}

void loop() {
  uint32_t orange = pixels.Color(255, 40, 0);

  if (digitalRead(LEFT_BUTTON_PIN) == HIGH) {
    blinkHalf(firstHalfStart, firstHalfEnd, orange);
  }

  if (digitalRead(RIGHT_BUTTON_PIN) == HIGH) {
    blinkHalf(secondHalfStart, secondHalfEnd, orange);
  }

  // Default state: all pixels off
  pixels.clear();
  pixels.show();
  delay(50); // Small delay to prevent button bounce issues and rapid looping
}
