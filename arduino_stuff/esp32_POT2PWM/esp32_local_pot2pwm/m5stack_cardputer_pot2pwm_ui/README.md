# M5Stack Cardputer POT2PWM UI

This sketch displays telemetry from `esp32_local_pot2pwm` and sends manual
override commands back to the controller over ESP-NOW broadcast.

## Link

No UART wiring or MAC configuration is needed. Both sketches use ESP-NOW on
Wi-Fi channel 1 and broadcast their packets.

## Controls

- Up arrow: previous row
- Down arrow: next row
- Enter: enable/clear manual override on the selected editable row
- Left arrow: decrease selected manual value
- Right arrow: increase selected manual value
- Backspace: clear selected override

In the M5Cardputer keyboard library, the arrow keys are reported as `;`, `.`,
`,` and `/`, which the sketch maps to up, down, left and right.

## Build

```sh
arduino-cli compile --fqbn esp32:esp32:m5stack_cardputer m5stack_cardputer_pot2pwm_ui
```
