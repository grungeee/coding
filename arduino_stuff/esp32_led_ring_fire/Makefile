BOARD_FQBN ?= esp32:esp32:esp32c3
PORT ?= /dev/ttyUSB0
SKETCH ?= esp32_led_ring_fire.ino

.PHONY: bootstrap lint format compile upload monitor test clean

bootstrap:
	./scripts/bootstrap.sh

lint:
	arduino-lint --fix $(SKETCH)

format:
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i $$(find . -name '*.ino' -o -name '*.h' -o -name '*.cpp'); \
	else \
		echo "clang-format not found; install it or skip formatting."; \
	fi

compile:
	arduino-cli compile --fqbn $(BOARD_FQBN) $(SKETCH)

upload:
	arduino-cli upload --fqbn $(BOARD_FQBN) --port $(PORT) $(SKETCH)

monitor:
	arduino-cli monitor --port $(PORT) --config baud=115200

test:
	@if [ -f platformio.ini ]; then \
		pio test; \
	else \
		echo "No PlatformIO project detected; add platformio.ini to enable unit tests."; \
	fi

clean:
	rm -rf build
