/*
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <WiFi.h>
#include <PubSubClient.h>

// --------------- BME280 ---------------
#include <Wire.h>
#include <Adafruit_Sensor.h>



//const char* ssid = "pls-connect-O3O";
//const char* password = "jesdod-qirZoj-mysxe5";
//const char* mqtt_server = "10.0.0.198";

const char* ssid = "hello ( ͡° ͜ʖ ͡°)";
const char* password = "HustleCastel";
const char* mqtt_server = "10.0.0.198";

const int LED_PIN_BUILTIN = 8; // Default LED pin 2 on ESP32; 8 on ESP32c3

WiFiClient espClient;
PubSubClient client(espClient);

const char* publishTopic = "esp32/data";
const char* subscribeTopic = "esp32/command";

const char* output = "ESP32 is running";

void callback(char* topic, byte* payload, unsigned int length) {
  String message = String((char*)payload, length);
  Serial.println("Received message: " + message + " on topic: " + String(topic));

  // Control the LED based on the message
  if (message == "on") {
    digitalWrite(LED_PIN_BUILTIN, HIGH);
    Serial.println("LED turned ON");
  } else if (message == "off") {
    digitalWrite(LED_PIN_BUILTIN, LOW); Serial.println("LED turned OFF"); } }

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), "mqtt", "mqtt")) {
      Serial.println("connected");
      // Resubscribe to the topic after reconnecting
      client.subscribe(subscribeTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(LED_PIN_BUILTIN, OUTPUT);
  digitalWrite(LED_PIN_BUILTIN, LOW);

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Configure MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  // Keep the MQTT connection alive
  client.loop();

  // Publish a message periodically
  static unsigned long lastPublish = 0;
  if (millis() - lastPublish > 5000) { // Publish every 5 seconds
    client.publish(publishTopic, output);
    Serial.println("Message published: " + String(output));
    lastPublish = millis();
  }
}

*/




#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <WiFi.h>
#include <PubSubClient.h>

// --------------- BME280 ---------------
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;
#define BME280_ADDRESS 0x76 //or 0x77


const char* ssid = "hello ( ͡° ͜ʖ ͡°)";
const char* password = "HustleCastel";
const char* mqtt_server = "192.168.31.115";

const int LED_PIN_BUILTIN = 2; // Default LED pin 2 on ESP32; 8 on ESP32c3
//const int TWO_CORLOR_LED = 7; // esp32c3

WiFiClient espClient;
PubSubClient client(espClient);

//const char* publishTopicString = "homeassistant/esp32/message";

//const char* publishTopicTemperature = "homeassistant/esp32/temperature";
//const char* publishTopic= "homeassistant/esp32/data";
//const char* subscribeTopic = "homeassistant/esp32/command";

const char* publishTopic= "esp32/data";
const char* subscribeTopic = "esp32/command";

const char* output = "ESP32 is running";


void callback(char* topic, byte* payload, unsigned int length) {
  String message = String((char*)payload, length);
  Serial.println("Received message: " + message + " on topic: " + String(topic));

  // Control the LED based on the message
  if (message == "on") {
    digitalWrite(LED_PIN_BUILTIN, HIGH);
    Serial.println("LED turned ON");
  } else if (message == "off") {
    digitalWrite(LED_PIN_BUILTIN, LOW);
    Serial.println("LED turned OFF");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), "mqtt", "mqtt")) {
      Serial.println("connected");
      // Resubscribe to the topic after reconnecting
      client.subscribe(subscribeTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(LED_PIN_BUILTIN, OUTPUT);
  digitalWrite(LED_PIN_BUILTIN, LOW);


  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Configure MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);


// --------------- BME280 ---------------
// Initialize the sensor
  if (!bme.begin(BME280_ADDRESS)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    delay(2000);
    ESP.restart();
  }

  /// end setup ///
}

void loop() {
// --------------- MQTT ---------------
  if (!client.connected()) {
    reconnect();
  }
  // Keep the MQTT connection alive
  client.loop();

// --------------- BME280 ---------------
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F; //hPa

// --------------- JSON ----------------
  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["pressure"] = pressure;

  char jsonBuffer[64]; // Adjust the buffer size as needed, e.g., 512
  serializeJson(doc, jsonBuffer);


  // ====> Publish
  static unsigned long lastPublish = 0;
  if (millis() - lastPublish > 5000) { // Publish every 5 seconds
    client.publish(publishTopic, jsonBuffer);
    Serial.println("Message published: " + String(jsonBuffer));
    delay(500);
    //client.publish(publishTopicString, "666");

    
    lastPublish = millis();
  }
}
// end loop


