#include <WiFi.h>

// Access Point credentials
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// Start a WiFi server on port 80
WiFiServer server(80);

// Define output pins (change these if needed)
const int controlPin1 = 6;  // will be set HIGH on "BUTTON1" command
const int controlPin2 = 7;  // will be set HIGH on "BUTTON2" command

void setup() {
  Serial.begin(115200);

  // Initialize control pins as outputs and set them LOW
  pinMode(controlPin1, OUTPUT);
  pinMode(controlPin2, OUTPUT);
  digitalWrite(controlPin1, LOW);
  digitalWrite(controlPin2, LOW);

  // Start the access point
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // Start the server
  server.begin();
  Serial.println("Server started");
}

void loop() {
  // Wait for a client connection
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Client connected");
    // Read the command sent by the client (up to newline)
    String command = client.readStringUntil('\n');
    command.trim();  // Remove any extra whitespace/newlines
    Serial.print("Received command: ");
    Serial.println(command);

    // Check which command was received
    if (command == "BUTTON1") {
      bool currentState = digitalRead(controlPin1);
      digitalWrite(controlPin1, currentState ? LOW : HIGH);
      //delay(1000);
      //digitalWrite(controlPin1, LOW);
    } else if (command == "BUTTON2") {
      bool currentState = digitalRead(controlPin1);
      digitalWrite(controlPin2, currentState ? LOW : HIGH);
      //digitalWrite(controlPin2, HIGH);
      //delay(1000);
      //digitalWrite(controlPin2, LOW);
    }
    
    // Optionally, you can send a reply back to the client:
    /*
    client.println("Command received");
    delay(10);
    client.stop();
    Serial.println("Client disconnected");
    */
  }
}
