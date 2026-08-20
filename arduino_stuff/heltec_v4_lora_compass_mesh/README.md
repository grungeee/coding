# Heltec V4 LoRa Compass Mesh

Custom low-latency position mesh for Heltec ESP32-S3 + SX1262 boards. Each node broadcasts a compact binary LoRa packet with its unique node ID and GNSS position, stores origin positions in a peer table, forwards unseen packets for a bounded number of hops, computes distance and bearing, then lights a 24-pixel NeoPixel ring in the peer directions after subtracting the local compass heading. Navigation calculations use a fixed local WGS84 ENU (East-North-Up) frame in metres, not direct latitude/longitude arithmetic.

This is intentionally not Meshtastic. It is a simple broadcast mesh: every node sends its own position on a jittered interval and listens for everyone else.

## Hardware

- Heltec WiFi LoRa 32 V4 with L76K GNSS module, or Heltec Wireless Tracker profile.
- On-board 128x64 OLED display on the V4 display bus.
- 24 LED WS2812/NeoPixel ring.
- Compass module for absolute heading. The sketch defaults to Adafruit BNO055 over I2C. Without it, it falls back to GNSS course while moving.
- LoRa antenna connected before transmitting.
- LED ring power note: place a 470-1000 uF capacitor across LED power/GND near the ring and put about a 330 ohm series resistor on DIN.

## Battery Indicator

On the Heltec V4 profile, the firmware samples the built-in battery divider on GPIO1 and drives GPIO37 only for the short measurement. The OLED header shows `Bxx%` and a filled battery icon; serial status reports the estimated millivolts and percentage. The same millivolts value is included in outbound mesh packets and sets the low-battery flag below `BATTERY_LOW_MV`.

The charger-status output on this board drives its own red LED, rather than an ESP32 GPIO. Therefore the OLED shows charge level by default but cannot truthfully show an active charging bolt. `BATTERY_CHARGE_DETECT_PIN` is reserved in `config.h` for a later wired status signal. Adjust the sample interval, divider calibration, and empty/full/low thresholds in that file after comparing the reading with a multimeter.

## Libraries

Install these in Arduino IDE Library Manager:

- RadioLib
- TinyGPSPlus
- Adafruit NeoPixel
- Adafruit SSD1306
- Adafruit BNO055
- Adafruit Unified Sensor

The local `arduino_stuff` tree already has TinyGPSPlus, Adafruit NeoPixel, and BNO055 copies in places, but Library Manager installs are easier to keep consistent.

## Configure Each Node

Open `config.h` and set:

- `NODE_ID`: set a non-zero unique ID if desired; `0` derives one from each ESP32 factory MAC so identical firmware can be flashed to multiple devices.
- `BOARD_PROFILE`: `BOARD_PROFILE_HELTEC_WIFI_LORA_32_V4` or `BOARD_PROFILE_HELTEC_WIRELESS_TRACKER`.
- `LORA_FREQ_MHZ`: legal frequency for your region. The default is `915.0` MHz for US ISM testing; change it for EU868 or other regions.
- `LORA_TX_POWER_DBM`: stay inside local rules and board limits.
- `LED_RING_PIN`: DIN pin for the external 24 LED ring.
- `USE_BNO055_COMPASS`: set to `0` if the compass is not installed yet.

Default WiFi LoRa 32 V4 pins:

| Function | GPIO |
| --- | ---: |
| LoRa NSS/SCK/MOSI/MISO/RST/BUSY/DIO1 | 8/9/10/11/12/13/14 |
| GNSS ESP32 RX/TX | 38/39 |
| GNSS power control | 34, active-low |
| GNSS reset/wake/PPS | 42/40/41 |
| BNO055 I2C SDA/SCL | 3/4 |
| On-board OLED SDA/SCL | 17/18 |
| OLED/Vext power control | 36, active-low |
| OLED reset | 21 |
| LED ring DIN | 45 |

Default Wireless Tracker pins:

| Function | GPIO |
| --- | ---: |
| LoRa NSS/SCK/MOSI/MISO/RST/BUSY/DIO1 | 8/9/10/11/12/13/14 |
| GNSS ESP32 RX/TX | 33/34 |
| GNSS power control | 3 |
| GNSS reset | 35 |
| I2C SDA/SCL | 5/6 |
| LED ring DIN | 18 |

On the exact V4 pin map, GPIO42 is `GPS_RST`, and GPIO36 is active-low `Vext_Ctrl` for the OLED and LoRa front end, so neither is safe for ring DIN. GPIO32 is reserved by the ESP32-S3 flash/PSRAM interface and causes boot watchdog resets. GPIO45 is the safe ring data pin in this sketch. GPIO3 and GPIO4 are exposed GPIOs and are reserved here for BNO055 SDA and SCL. Heltec has changed GNSS power and connector details between revisions, so verify the board revision before soldering.

The GNSS UART names are from the ESP32's perspective: GPIO38 is ESP32 RX and receives L76K TX; GPIO39 is ESP32 TX and drives L76K RX. The sketch therefore calls `HardwareSerial.begin(9600, SERIAL_8N1, 38, 39)`. GPIO34 enables GNSS with a low level, GPIO42 reset is pulsed low at startup and then released high, and GPIO40 is held high to keep the L76K awake.

## Mesh Forwarding

Packets are position advertisements identified by `(origin node ID, sequence)`. Each node accepts a newer sequence once, stores it in `PeerTable`, and queues one delayed rebroadcast. The packet carries a hop limit of three and a hop count, so peers can be reached through intermediate nodes without an unlimited broadcast storm. Duplicate and out-of-order packets are dropped.

All nodes must use the same mesh firmware, frequency, bandwidth, spreading factor, coding rate, and sync word. Because the packet version changed for hop fields, update every node before testing multi-hop behavior.

## Packet Format

All multibyte fields are sent in the ESP32's native little-endian format. The packet is fixed at 40 bytes:

| Field | Type | Notes |
| --- | --- | --- |
| magic | uint16 | `0x464D` |
| version | uint8 | currently `2` |
| type | uint8 | `1` = position |
| nodeId | uint32 | configured unique ID |
| sequence | uint16 | increments per TX |
| uptimeMs | uint32 | sender millis |
| latE7/lonE7 | int32 | degrees * 1e7 |
| altM | int16 | altitude in meters |
| speedCentiKph | uint16 | GNSS speed * 100 |
| courseCdeg | uint16 | GNSS course degrees * 100 |
| headingCdeg | uint16 | compass heading degrees * 100 |
| hdopCenti | uint16 | TinyGPSPlus HDOP value |
| sats | uint8 | satellites visible |
| flags | uint8 | bit 0 GNSS valid, bit 1 compass/course valid, bit 2 low battery |
| batteryMv | uint16 | `0` when disabled |
| hopLimit/hopCount | uint8/uint8 | remaining forwarding budget and traversed hops |
| crc | uint16 | CRC-16/CCITT over the full packet with crc zeroed |

RadioLib also enables LoRa CRC. The packet CRC is still useful to reject stale or malformed same-channel traffic.

## Scheduling And Peer Handling

- GNSS bytes are consumed continuously in `loop()`.
- RX uses SX1262 DIO1 interrupt + `radio.readData()`.
- TX happens every `TX_INTERVAL_MS` plus random `TX_JITTER_MS` to reduce collisions. The default is a conservative 15 seconds for the EU/Austria 868 MHz profile.
- Peers expire after `PEER_TIMEOUT_MS`.
- LED frames render every `LED_FRAME_MS`.
- There are no `delay()` calls in the main loop. LoRa `transmit()` occupies the radio for the short packet airtime, then RX is restarted.

## Direction Stability

- The first local GNSS fix establishes a stable WGS84 ENU origin. Local and peer positions are converted to that frame before filtering, distance, bearing, or LED selection.
- Local and peer positions use a lightweight alpha-beta constant-velocity filter. Compass heading and peer bearing use circular unit-vector filters, so smoothing remains correct across 359 to 0 degrees.
- LED selection retains its current pixel until the filtered bearing has crossed the normal sector boundary plus `LED_DIRECTION_HYSTERESIS_DEG`.
- Newer packets with a non-increasing sender uptime are rejected as stale. Valid GNSS packets that exceed `POSITION_JUMP_ALLOWANCE_M + POSITION_MAX_SPEED_MPS * elapsed_seconds` are rejected before changing a peer position.
- A peer enters the nearby-uncertain state when its filtered range is less than the larger of `CLOSE_RANGE_THRESHOLD_M` and the two nodes' combined estimated GNSS uncertainty, capped by `CLOSE_RANGE_MAX_TRIGGER_M`. The cap prevents an absent or poor HDOP reading from falsely treating a distant peer as close. It keeps its last reliable single-pixel bearing and blinks faster. It leaves that state only after `CLOSE_RANGE_EXIT_HYSTERESIS_M` of additional separation.
- A non-near peer remains a directional dot. A nearby peer is also only a directional dot, using its held bearing so close GNSS noise cannot rotate it. The global display sequence shows one peer at a time.

All of these values are defined together near the top of `config.h`, so they can be tuned at the test site without changing the mesh protocol.

## LED Behavior

- With no GNSS fix, LED 0 pulses orange.
- A red dot marks north relative to the current heading.
- Every active peer with a valid GNSS fix gets a colored dot at its relative bearing.
- Festival wearables use fixed colors: HT-1 green, HT-2 blue, HT-3 purple.
- A peer reached through another node is still displayed using its origin coordinates; Serial status includes its hop count.
- Color is derived from `nodeId`; brightness fades with age and distance.
- Only one peer directional pixel is shown at a time. Each peer gets a complete four-second cycle: a `PEER_DISPLAY_HOLD_MS` light-up/hold, a `PEER_DISPLAY_FADE_MS` ramp-down, and a dark `PEER_DISPLAY_GAP_MS` before the next peer begins. Peers never blend or appear concurrently.
- At boot, the 24-pixel ring sweeps from LED 0 to LED 23. Each new LED starts at maximum configured brightness and every already-lit LED retains 75% of its previous brightness on each step.

## OLED Status

The V4 OLED uses its own I2C bus on GPIO17/GPIO18, leaving GPIO3/GPIO4 dedicated to the BNO055. The screen refreshes twice per second and shows GNSS state and byte count, latitude/longitude, compass heading, LoRa state, TX count, accepted RX count, active peer count, and up to three peer IDs with distance and absolute bearing. The L76K UART follows the working Meshtastic V4 assignment: ESP32-S3 RX GPIO39 and TX GPIO38.

## Upload

Build each wearable with a stable mesh ID. The IDs below are deliberately
different so devices running identical firmware never appear as the same peer:

| Wearable | Build flag |
| --- | --- |
| HT-1 | `-DNODE_ID=0x0000A101` |
| HT-2 | `-DNODE_ID=0x0000A102` |
| HT-3 | `-DNODE_ID=0x0000A103` |

GPIO45 is the WS2812/NeoPixel data pin. Connect ring DIN to GPIO45, use a
common ground, and power the ring from a supply sized for 24 pixels.

Use `compiler.cpp.extra_flags` for the ID flag. Do not override
`build.extra_flags`: that replaces ESP32 board definitions required by the
Arduino core.

```sh
SKETCH=/Users/grungeee/Documents/coding/arduino_stuff/heltec_v4_lora_compass_mesh
arduino-cli compile --fqbn esp32:esp32:esp32s3 \
  --build-property compiler.cpp.extra_flags=-DNODE_ID=0x0000A101 \
  --build-path /tmp/heltec_v4_lora_compass_mesh_ht1 "$SKETCH"
arduino-cli upload --fqbn esp32:esp32:esp32s3 -p /dev/cu.usbmodem101 \
  --input-dir /tmp/heltec_v4_lora_compass_mesh_ht1 "$SKETCH"
```

1. Select an ESP32-S3 Heltec board profile that matches your installed Heltec core.
2. Select the right flash size and LoRa FEM option for the exact WiFi LoRa 32 V4 revision.
3. Edit `config.h` for each node.
4. Flash at least two nodes with different `NODE_ID` values.
5. Open Serial Monitor at `115200` baud and wait for `radio=ok`, `fix=yes`, and peer lines.

For fast festival testing, start with two nodes outside with antennas attached, verify Serial peer distance/bearing first, then calibrate the compass and LED index orientation on the enclosure.
