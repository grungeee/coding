# ESP32 38-pin DevKit — IDC-routed pinout (JTAG-free)

Target: `esp32_38pin_local_pot2pwm.ino` (classic ESP32, ADC1 pots, ESP-NOW to Cardputer).
Design rules enforced here:
- no strapping pin carries a signal that can change boot state (GPIO0/2/5/12/15 all kept out of the signal path)
- **no JTAG pin used at all: GPIO12 and GPIO14 stay unconnected** (14 = MTMS, had its own reset behaviour)
- pots stay on ADC1 (32-39) so WiFi/ESP-NOW keeps working (ADC2 dies with the radio up)
- every signal pair is adjacent on the DevKit header, so the ribbon leaves the board as pairs

## Signal table (IDC order)

| IDC pin | Signal | GPIO | Header position |
|---|---|---|---|
| 1 | PWM1 (board1-A) | 32 | left, adjacent 32/33 |
| 2 | PWM2 (board1-B) | 33 | left |
| 3 | switch1 out A (binary) | 25 | left, adjacent 25/26 |
| 4 | switch1 out B (binary) | 26 | left |
| 5 | PWM3 (board2-A) | 27 | left |
| 6 | PWM4 (board2-B) | 21 | right (isolated pin, no pair left) |
| 7 | switch2 out A (binary) | 23 | right, adjacent 23/22 |
| 8 | switch2 out B (binary) | 22 | right |
| 9 | PWM5 (board3-A) | 19 | right, adjacent 19/18 |
| 10 | PWM6 (board3-B) | 18 | right |
| 11 | switch3 out A (binary) | 17 | right, adjacent 17/16 |
| 12 | switch3 out B (binary) | 16 | right |
| 13 | switch1 in | 4 | right (INPUT_PULLDOWN, ON = 3.3 V) |
| 14 | switch2 in | 13 | left (INPUT_PULLDOWN, ON = 3.3 V) |
| 15 | switch3 in | 15 | right (INPUT_PULLDOWN, ON = 3.3 V) |
| 16 | 3V3 | - | |
| 17 | GND | - | |
| 18 | GND | - | |
| 19 | GND | - | |
| 20 | 5V (spare) | - | |

Lone pins: GPIO14 and GPIO12 — do not crimp them into the connector at all.

## Sketch constants

    const int potPins[3] = {36, 35, 34};
    const int switchInputPins[3] = {4, 13, 15};
    const int pwmPins[6] = {32, 33, 27, 21, 19, 18};
    const int digitalOutPins[6] = {25, 26, 23, 22, 17, 16};
    const int switchDigitalOutPins[3][2] = {
      {25, 26},
      {23, 22},
      {17, 16}
    };

## Boot-safety notes

- GPIO32/33/25/26/27/23/22/21/19/18/17/16/4: no reset role, no internal pull-up/down — quiet at power-up.
- GPIO13 (MTCK): JTAG pin, **not** a strapping pin, so it cannot change the boot. Cost: no hardware JTAG while used.
- GPIO15 (MTDO): strapping pin with a default internal pull-up -> boots normally with the switch open or closed.
  Only side effect: if it is ever shorted to GND during reset the boot log is suppressed (boot itself is unaffected).
  The code enables INPUT_PULLDOWN on it, so the ON/OFF logic is the same as the other switches.
- GPIO21 used as PWM4 is boot-neutral (default I2C SDA, no strapping role).
- Zero-extra-parts alternative for switch3 in: GPIO39 (VN) instead, which has no boot role and no
  internal pulls at all — but it then needs an external 10k pull-down to GND to keep the
  "ON = connected to 3.3 V" logic.
- ESP32-S3 variant has none of this trouble: its only strapping pins are 0/3/45/46 and JTAG is 39-42,
  so 14 is free there.

## Header reference (usable pins only)

    LEFT  J2: 3V3 EN | 36 39 34 35 | 32 33 | 25 26 27 | 14 [12] GND | 13 | 9 10 11 5V
    RIGHT J3: GND | 23 22 | [1] [3] | 21 | GND | 19 18 | [5] | 17 16 | 4 | [0] [2] [15] | 8 7 6

    [] = strapping / USB-serial / flash pin, not used

## Two-column (connector) view

Physical IDC view, pin 1 top-left: odd pins = row A, even pins = row B,
columns numbered 1-10 (col n = pins 2n-1 / 2n).

    cols 1-5      col 1         col 2         col 3         col 4         col 5
      row A     | 1  PWM1  32 | 3  DO1a  25 | 5  PWM3  27 | 7  DO2a  23 | 9  PWM5  19
      row B     | 2  PWM2  33 | 4  DO1b  26 | 6  PWM4  21 | 8  DO2b  22 | 10 PWM6  18

    cols 6-10     col 6         col 7         col 8         col 9         col 10
      row A     | 11 DO3a  17 | 13 sw1 in  4 | 15 sw3 in 15 | 17 GND     | 19 GND
      row B     | 12 DO3b  16 | 14 sw2 in 13 | 16 3V3     | 18 GND     | 20 5V spare

Blocks: switch1 = pins 1-4 (2 PWM + 2 binary), switch2 = pins 5-8, switch3 = pins 9-12,
switch inputs grouped on pins 13-15, power on 16-20.
Row A of every block is adjacent to row B, so each PWM sits next to its switch output
(pin 1 next to pin 3, pin 2 next to pin 4 - the layout you described).

## Compact two-column form (2 columns x 10 rows)

    |1: PWM1  32|2: PWM2  33|
    |3: DO1a  25|4: DO1b  26|
    |5: PWM3  27|6: PWM4  21|
    |7: DO2a  23|8: DO2b  22|
    |9: PWM5  19|10: PWM6  18|
    |11: DO3a 17|12: DO3b 16|
    |13: sw1 in 4|14: sw2 in 13|
    |15: sw3 in 15|16: 3V3|
    |17: GND|18: GND|
    |19: GND|20: 5V|
