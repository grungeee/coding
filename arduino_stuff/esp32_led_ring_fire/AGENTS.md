# Repository Guidelines
This repository hosts a minimal Arduino sketch for an ESP32-driven LED ring. Follow these guidelines to keep new contributions consistent with the expected tooling, hardware assumptions, and documentation standards.

## Project Structure & Module Organization
- `esp32_led_ring_fire.ino`: main sketch containing `setup()` and `loop()`. Place new helpers above `loop()` or break complex logic into `.h/.cpp` pairs under `src/`.
- `lib/` (optional): vendor reusable libraries here and note their provenance in a short README fragment.
- `assets/` (optional): store schematics, pinouts, or photos; keep binaries small and add context in accompanying markdown.

## Build, Test, and Development Commands
- `arduino-cli core install esp32:esp32`: install the Espressif toolchain before first build.
- `arduino-cli compile --fqbn esp32:esp32:esp32 esp32_led_ring_fire.ino`: compile the sketch for the default ESP32 Dev Module.
- `arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 esp32_led_ring_fire.ino`: flash the board; update `--port` for your USB/serial path.
- `arduino-lint --fix esp32_led_ring_fire.ino`: run basic static checks; re-run lint after major edits.

## Coding Style & Naming Conventions
- Indent with two spaces, use K&R braces, and avoid trailing commas in initializer lists.
- Name functions and locals in lowerCamelCase, constants in ALL_CAPS, and favor descriptive pin aliases (`ledRingPin`).
- Encapsulate hardware-specific values in `constexpr` or `config.h`; keep secrets in `secrets.h` excluded from version control.
- Add concise inline comments for non-obvious timing, math, or LED palette logic.

## Testing Guidelines
- Factor pure logic into helper functions so they can be unit-tested with desktop toolchains.
- For regression checks, add Arduino Unity tests under `test/` and run via `pio test` or a lightweight `gcc` harness when available.
- When hardware validation is required, document manual test steps in the PR and capture expected LED behavior (photos or serial dumps).

## Commit & Pull Request Guidelines
- Write commits in present-tense imperatives (`Add gamma correction table`), keep subjects ≤72 characters, and reference issue IDs when relevant.
- Ensure PRs describe hardware setup (board revision, ring size, power source), include the compile/upload command output, and attach visuals when behavior changes.
- Flag breaking changes or pin remaps explicitly so wiring diagrams and downstream sketches can be updated promptly.

## Security & Configuration Tips
- Never hardcode Wi-Fi credentials; load them from `secrets.h` and share patterns via `secrets.example.h`.
- Confirm new libraries are compatible with the targeted ESP32 core version and document required `platform.txt` tweaks or menuconfig changes in the PR.
