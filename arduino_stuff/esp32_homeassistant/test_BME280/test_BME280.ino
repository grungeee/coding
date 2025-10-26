#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Create an object for the sensor
Adafruit_BME280 bme;

// Address can be 0x76 or 0x77
#define BME280_ADDRESS 0x76

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize the sensor
  if (!bme.begin(BME280_ADDRESS)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

void loop() {
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  delay(2000); // Wait for 2 seconds
}
