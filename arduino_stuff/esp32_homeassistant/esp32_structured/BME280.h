#ifndef BME280_H
#define BME280_H

#include <Adafruit_BME280.h>

extern Adafruit_BME280 bme;

void setupBME280();
void readBME280(float &temperature, float &humidity, float &pressure);

#endif
