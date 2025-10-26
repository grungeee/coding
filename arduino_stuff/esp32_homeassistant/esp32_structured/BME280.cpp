#include "BME280.h"

Adafruit_BME280 bme;
#define BME280_ADDRESS 0x76

void setupBME280() {
  if (!bme.begin(BME280_ADDRESS)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    delay(2000);
    ESP.restart();
  }
}

void readBME280(float &temperature, float &humidity, float &pressure) {
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
}
