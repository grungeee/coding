#include <Arduino.h>
#include "driver/ledc.h"  // ESP32 LEDC PWM driver

const int pwmPin = 25;  // Change this to your desired pin
const int pwmFreq = 1000;  // 1kHz base frequency
const ledc_timer_bit_t pwmResolution = LEDC_TIMER_8_BIT;  // Correct type
const int sineTableSize = 100;  // Steps for the sine wave
int sineTable[sineTableSize];  // Lookup table for sine wave

void generateSineTable() {
    for (int i = 0; i < sineTableSize; i++) {
        float angle = (2.0 * PI * i) / sineTableSize;
        sineTable[i] = (sin(angle) * 127.5) + 127.5;  // Maps sine from 0-255
    }
}

void setup() {
    Serial.begin(115200);  // Debug output

    // Configure the PWM timer
    ledc_timer_config_t timerConfig = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = pwmResolution,  // Now correctly defined
        .timer_num = LEDC_TIMER_0,
        .freq_hz = pwmFreq,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timerConfig);

    // Configure the PWM channel
    ledc_channel_config_t channelConfig = {
        .gpio_num = pwmPin,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 127,  // Mid-point duty cycle
        .hpoint = 0
    };
    ledc_channel_config(&channelConfig);

    generateSineTable();
}

void loop() {
    static int index = 0;
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, sineTable[index]);  // Set duty cycle
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);  // Apply the update
    index = (index + 1) % sineTableSize;  // Loop through sine wave table
    delayMicroseconds(1000);  // Adjust for frequency control
}
