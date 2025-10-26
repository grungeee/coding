# Development Environment Setup

Follow these steps to prepare a workstation for the ESP32 LED Ring Fire sketch.

## Prerequisites
- `arduino-cli` 0.34 or newer on your `PATH`.
- USB access to an ESP32 development board and LED ring.
- Optional: PlatformIO Core (`pio`) if you maintain unit tests in a separate PlatformIO project.

## Quickstart
1. Clone the repository and bootstrap the Arduino environment:
   ```bash
   git clone <repo-url>
   cd esp32_led_ring_fire
   make bootstrap
   ```
   The bootstrap script initializes `arduino-cli` and installs the Espressif core if it is missing.
2. List supported boards to confirm installation:
   ```bash
   arduino-cli board listall | grep ESP32
   ```
3. Update `PORT` in the `Makefile` if your development board enumerates on a different serial device.
4. Build the sketch:
   ```bash
   make compile
   ```

## Uploading Firmware
1. Put the board in bootloader mode if your model requires it (many boards need the `BOOT` button held during reset).
2. Flash the sketch:
   ```bash
   make upload PORT=/dev/ttyUSB0
   ```
3. Monitor serial output or LED behavior to confirm success:
   ```bash
   make monitor PORT=/dev/ttyUSB0
   ```

## Troubleshooting
- Run `arduino-cli core update-index` if `esp32:esp32` fails to download or appears outdated.
- Add third-party libraries to `lib/` and note their source in `AGENTS.md` so others can rehydrate the environment.
- If flashing fails due to permissions on Linux, add your user to the `dialout` group and reconnect the device.
