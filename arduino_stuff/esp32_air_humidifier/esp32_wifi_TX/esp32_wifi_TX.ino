#include <WiFi.h>

// Access Point credentials (must match the receiver's settings)
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// Define the server IP and port (AP default IP is typically 192.168.4.1)
IPAddress serverIP(192, 168, 4, 1);
const uint16_t serverPort = 80;

// Define button pins
const int buttonPin1 = 23;  // When pressed, send "BUTTON1"
const int buttonPin2 = 22;  // When pressed, send "BUTTON2"

void setup() {
  Serial.begin(115200);
  
  // Initialize button pins as inputs with internal pull-up resistors.
  // (Assumes the buttons connect the pin to GND when pressed.)
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);

  // Connect to the WiFi network (the AP hosted by the receiver)
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check if Button1 is pressed (active LOW)
  if (digitalRead(buttonPin1) == LOW) {
    Serial.println("Button1 pressed");
    WiFiClient client;
    if (client.connect(serverIP, serverPort)) {
      client.println("BUTTON1");
      client.stop();
      Serial.println("Command sent: BUTTON1");
    } else {
      Serial.println("Connection to server failed");
    }
    delay(500);  // Simple debounce/delay
  }
  
  // Check if Button2 is pressed (active LOW)
  if (digitalRead(buttonPin2) == LOW) {
    Serial.println("Button2 pressed");
    WiFiClient client;
    if (client.connect(serverIP, serverPort)) {
      client.println("BUTTON2");
      client.stop();
      Serial.println("Command sent: BUTTON2");
    } else {
      Serial.println("Connection to server failed");
    }
    delay(500);  // Simple debounce/delay
  }
  
  delay(50);  // Loop delay
}
