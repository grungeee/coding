#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display settings (change dimensions if needed)
#define SCREEN_WIDTH 128 // OLED display width in pixels
#define SCREEN_HEIGHT 32 // OLED display height in pixels
#define OLED_RESET    -1 // Reset pin (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Ultrasonic sensor pins (adjust to your wiring)
const int trigPin = 5;   // Trigger pin
const int echoPin = 18;  // Echo pin

// Global Variables
float distance_cm = 0;

// Counter variables
int count = 0; 
bool starting_position = false;
bool up = false;
bool down = false;
int ref_distance = 0;

void setup() {
  // Start serial communication for debugging
  Serial.begin(115200);
  
  // Initialize OLED display with I2C address 0x3C (common for SSD1306)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true); // Loop forever if OLED initialization fails
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
  
  // Initialize sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  // Ensure trigger is LOW
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Trigger the ultrasonic burst
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Measure the time for the echo to be received
  long duration = pulseIn(echoPin, HIGH);
  
  // Calculate distance in centimeters:
  // Speed of sound is ~343 m/s (0.0343 cm/µs); divide by 2 for round trip
  distance_cm = duration * 0.0343 / 2;
  
  update_oled_display_distance(); 
  update_oled_display_counter(); 

  // =============< Counter >==============
  if (distance_cm < 70) {
    starting_position = true;
  }

  if (starting_position) {
    if (distance_cm < 20) {
      starting_position = false;
      down = true;
      Serial.print("DOWN ");
    }
    if (distance_cm > 40 && distance_cm < 70) {
      up = true;
      Serial.print("UP ");
    }
    if (up == down && (up || down) != false) {
      up = false;
      down = false;
      count++;
      Serial.print("Count: ");
      Serial.println(count);
      delay(300);
      starting_position = true;
    }
  }
}

// =============< FUNCTIONS >==============
void update_oled_display_distance(){
  Serial.print("Distance: ");
  Serial.print(distance_cm);
  Serial.println(" cm");
  
  // Update OLED display DISTANCE
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Distance:");
  display.setCursor(0, 16);
  display.print(distance_cm);
  display.print(" cm");

  delay(250); // Update every 250 ms
}

void update_oled_display_counter(){
  // Update OLED display COUNT
  display.setCursor(64, 0);
  display.print("Count:");
  display.setCursor(64, 16);
  display.print(count);
  display.display();
}
