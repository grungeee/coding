#pragma once

// Set a non-zero ID to choose one manually. With zero, each ESP32 derives a
// unique mesh ID from its factory MAC address so identical firmware is safe.
// `-DNODE_ID=...` can assign a stable identity per uploaded wearable.
#ifndef NODE_ID
  #define NODE_ID 0UL
#endif

// Board profile:
//   1 = Heltec WiFi LoRa 32 V4 with L76K GNSS connector
//   2 = Heltec Wireless Tracker V1/V1.1 style board
#define BOARD_PROFILE_HELTEC_WIFI_LORA_32_V4 1
#define BOARD_PROFILE_HELTEC_WIRELESS_TRACKER 2
#define BOARD_PROFILE BOARD_PROFILE_HELTEC_WIFI_LORA_32_V4

// EU/Austria profile. 868.3 MHz is within the 868.0-868.6 MHz SRD band.
// The Heltec V4 has an external PA, so keep the SX1262 output conservative.
#define LORA_FREQ_MHZ 868.3
#define LORA_BANDWIDTH_KHZ 125.0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODING_RATE 5
#define LORA_SYNC_WORD 0x34
#define LORA_TX_POWER_DBM 0
#define LORA_PREAMBLE_LEN 8
#define LORA_TCXO_VOLTAGE 1.8

// A 15-second cadence leaves practical duty-cycle margin for direct peers and
// a small number of forwarded packets in the EU SRD band.
#define TX_INTERVAL_MS 15000UL
#define TX_JITTER_MS 250UL
#define PEER_TIMEOUT_MS 15000UL
#define LED_FRAME_MS 40UL
#define SERIAL_STATUS_MS 2000UL
#define DISPLAY_UPDATE_MS 500UL
#define FORWARD_DELAY_MIN_MS 30UL
#define FORWARD_DELAY_MAX_MS 140UL
#define MAX_FORWARD_QUEUE 8

// Direction stability. All position thresholds are in the local ENU frame,
// so they are expressed in metres rather than latitude/longitude units.
#define POSITION_FILTER_ALPHA 0.35
#define POSITION_FILTER_BETA 0.08
#define POSITION_FILTER_MAX_DT_SECONDS 30.0
#define HEADING_FILTER_ALPHA 0.25
#define BEARING_FILTER_ALPHA 0.22
#define LED_DIRECTION_HYSTERESIS_DEG 4.0
#define POSITION_JUMP_ALLOWANCE_M 20.0
#define POSITION_MAX_SPEED_MPS 15.0
#define GNSS_MIN_UNCERTAINTY_M 3.0
#define GNSS_UNKNOWN_UNCERTAINTY_M 15.0
#define GNSS_HDOP_TO_UNCERTAINTY_M 2.5
#define CLOSE_RANGE_THRESHOLD_M 8.0
#define CLOSE_RANGE_MAX_TRIGGER_M 10.0
#define CLOSE_RANGE_EXIT_HYSTERESIS_M 3.0
// One peer owns the directional display for a complete four-second cycle.
// The gap gives the previous pixel time to reach black before the next peer.
#define PEER_DISPLAY_HOLD_MS 1600UL
#define PEER_DISPLAY_FADE_MS 2000UL
#define PEER_DISPLAY_GAP_MS 400UL

// Heltec V4 external RF front-end controls. V4.2 uses GC1109; V4.3 uses
// KCT8103L. The firmware detects the installed part at startup.
#define LORA_FEM_POWER_PIN 7
#define LORA_FEM_ENABLE_PIN 2
#define LORA_FEM_GC1109_TX_PIN 46
#define LORA_FEM_KCT8103L_RXTX_PIN 5

// Display behavior.
#define LED_COUNT 24
#define LED_BRIGHTNESS 255
#define SHOW_NORTH_REFERENCE 1
#define MAX_PEERS 12

// Compass. A BNO055 on I2C gives an absolute heading. If you do not have it yet,
// set USE_BNO055_COMPASS to 0 and the sketch will fall back to GPS course while
// moving; the LED direction will be poor while standing still.
#define USE_BNO055_COMPASS 1
#define BNO055_I2C_ADDRESS 0x29
#define DISPLAY_I2C_ADDRESS 0x3C
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_RESET_PIN 21
#define DISPLAY_POWER_PIN -1
#define DISPLAY_POWER_ACTIVE_HIGH 0

#if BOARD_PROFILE == BOARD_PROFILE_HELTEC_WIFI_LORA_32_V4
  // SX1262 pins from Heltec WiFi LoRa 32 V4 pin map.
  #define LORA_NSS_PIN 8
  #define LORA_SCK_PIN 9
  #define LORA_MOSI_PIN 10
  #define LORA_MISO_PIN 11
  #define LORA_RST_PIN 12
  #define LORA_BUSY_PIN 13
  #define LORA_DIO1_PIN 14

  // Match the working Meshtastic Heltec V4 variant. HardwareSerial.begin()
  // receives these as (rx, tx), so the ESP32-S3 listens on GPIO39 and sends
  // optional L76K commands on GPIO38.
  #define GNSS_UART_RX_PIN 39
  #define GNSS_UART_TX_PIN 38
  #define GNSS_UART_RX_FALLBACK_PIN 38
  #define GNSS_POWER_PIN 34
  // Heltec/Meshtastic V4 GNSS enable is active-low.
  #define GNSS_POWER_ACTIVE_HIGH 0
  #define GNSS_RESET_PIN 42
  #define GNSS_RESET_ACTIVE_LOW 1
  #define GNSS_WAKE_PIN 40
  #define GNSS_PPS_PIN 41

  // Reserved for the external BNO055. GPIO3 (SDA) and GPIO4 (SCL) are
  // exposed header GPIOs on the V4 and are separate from the OLED bus.
  #define I2C_SDA_PIN 3
  #define I2C_SCL_PIN 4

  // On-board OLED bus. This is separate from the BNO055 bus above.
  #define DISPLAY_SDA_PIN 17
  #define DISPLAY_SCL_PIN 18

  // GPIO42 is GPS_RST and GPIO36 is Vext_Ctrl on this exact board. GPIO32 is
  // reserved by the ESP32-S3 flash/PSRAM interface. Use exposed GPIO45.
  #define LED_RING_PIN 45

  // Vext_Ctrl is active-low and must be asserted before using the OLED.
  #undef DISPLAY_POWER_PIN
  #define DISPLAY_POWER_PIN 36
#elif BOARD_PROFILE == BOARD_PROFILE_HELTEC_WIRELESS_TRACKER
  // SX1262 pins are the same on the Wireless Tracker pin map.
  #define LORA_NSS_PIN 8
  #define LORA_SCK_PIN 9
  #define LORA_MOSI_PIN 10
  #define LORA_MISO_PIN 11
  #define LORA_RST_PIN 12
  #define LORA_BUSY_PIN 13
  #define LORA_DIO1_PIN 14

  // Wireless Tracker V1.1: MCU UART2 RX on GPIO33, TX on GPIO34, VEXT/GNSS power on GPIO3.
  // ESP32-side UART pins passed to HardwareSerial.begin(rx, tx).
  #define GNSS_UART_RX_PIN 33
  #define GNSS_UART_TX_PIN 34
  #define GNSS_POWER_PIN 3
  #define GNSS_POWER_ACTIVE_HIGH 1
  #define GNSS_RESET_PIN 35
  #define GNSS_RESET_ACTIVE_LOW 1
  #define GNSS_WAKE_PIN -1
  #define GNSS_PPS_PIN -1

  // Pick free header pins for an external compass. GPIO18 is kept for LED_Write.
  #define I2C_SDA_PIN 5
  #define I2C_SCL_PIN 6
  #define LED_RING_PIN 18
#else
  #error "Unknown BOARD_PROFILE"
#endif

#define GNSS_BAUD 9600
#define GNSS_SERIAL_PORT 1
#define GNSS_WARMUP_MS 1000UL
#define GNSS_UART_RX_BUFFER_BYTES 1024
#define GNSS_UART_PROBE_MS 6000UL

// Battery monitoring. On Heltec WiFi LoRa 32 V4, GPIO1 is ADC1_CH0 behind a
// 390k/100k battery divider. GPIO37 must be high only while sampling it.
#if BOARD_PROFILE == BOARD_PROFILE_HELTEC_WIFI_LORA_32_V4
  #define BATTERY_ADC_PIN 1
  #define BATTERY_ADC_ENABLE_PIN 37
  #define BATTERY_ADC_ENABLE_ACTIVE_HIGH 1
  #define BATTERY_ADC_RATIO 4.9f
#else
  // Leave disabled unless the alternate board has a known battery divider.
  #define BATTERY_ADC_PIN -1
  #define BATTERY_ADC_ENABLE_PIN -1
  #define BATTERY_ADC_ENABLE_ACTIVE_HIGH 1
  #define BATTERY_ADC_RATIO 2.0f
#endif

#define BATTERY_ADC_SAMPLES 8
#define BATTERY_ADC_SETTLE_US 120
#define BATTERY_SAMPLE_MS 5000UL
#define BATTERY_EMPTY_MV 3300
#define BATTERY_FULL_MV 4200
#define BATTERY_LOW_MV 3400
// The V4 charger-status signal drives a hardware LED and is not connected to
// the ESP32. Set this to a wired status GPIO later to show a lightning bolt.
#define BATTERY_CHARGE_DETECT_PIN -1
#define BATTERY_CHARGE_DETECT_ACTIVE_LOW 1
