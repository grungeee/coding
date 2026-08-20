#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include <TinyGPSPlus.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>

#include "config.h"

#if USE_BNO055_COMPASS
  #include <Adafruit_BNO055.h>
  #include <Adafruit_Sensor.h>
#endif

#include "GeoMath.h"
#include "MeshPacket.h"
#include "PeerTable.h"
#include "PositionStability.h"

SX1262 radio = new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN);
TinyGPSPlus gps;
Adafruit_NeoPixel ring(LED_COUNT, LED_RING_PIN, NEO_GRB + NEO_KHZ800);
TwoWire displayWire(1);
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &displayWire, DISPLAY_RESET_PIN);

#if USE_BNO055_COMPASS
Adafruit_BNO055 bno = Adafruit_BNO055(55, BNO055_I2C_ADDRESS, &Wire);
#endif

PeerTable peers;
EnuFrame navigationFrame;
AlphaBetaPositionFilter localPositionFilter;
CircularAngleFilter headingFilter;
const PositionFilterConfig positionFilterConfig = {
    POSITION_FILTER_ALPHA,
    POSITION_FILTER_BETA,
    POSITION_FILTER_MAX_DT_SECONDS,
};

volatile bool radioRxFlag = false;
uint8_t rxBuffer[sizeof(MeshPacketV2)] = {0};
MeshPacketV2 forwardQueue[MAX_FORWARD_QUEUE] = {};
uint8_t forwardQueueHead = 0;
uint8_t forwardQueueTail = 0;
uint8_t forwardQueueCount = 0;
uint32_t nextForwardMs = 0;

uint32_t localNodeId = NODE_ID;
uint16_t txSequence = 0;
uint32_t nextTxMs = 0;
uint32_t nextLedMs = 0;
uint32_t nextStatusMs = 0;
uint32_t nextDisplayMs = 0;
uint32_t nextBatterySampleMs = 0;

bool radioReady = false;
bool displayReady = false;
bool compassReady = false;
bool gnssDataSeen = false;
uint32_t gnssBytesRead = 0;
int activeGnssRxPin = GNSS_UART_RX_PIN;
bool gnssFallbackRxActive = false;
uint32_t gnssUartStartedMs = 0;
enum class LoRaFemType : uint8_t { Unknown, GC1109, KCT8103L };
LoRaFemType loRaFemType = LoRaFemType::Unknown;
uint32_t radioTxCount = 0;
uint32_t radioRxCount = 0;
uint16_t ledTargetR[LED_COUNT] = {};
uint16_t ledTargetG[LED_COUNT] = {};
uint16_t ledTargetB[LED_COUNT] = {};
struct PeerLedCandidate {
  uint8_t led;
  uint32_t color;
};
PeerLedCandidate peerLedCandidates[MAX_PEERS] = {};
uint8_t peerLedCandidateCount = 0;
float ledShownR[LED_COUNT] = {};
float ledShownG[LED_COUNT] = {};
float ledShownB[LED_COUNT] = {};
double currentHeadingDeg = 0.0;

struct BatteryStatus {
  bool valid = false;
  bool charging = false;
  uint16_t millivolts = 0;
  uint8_t percent = 0;
};

BatteryStatus localBattery;

void initializeNodeId() {
  if (NODE_ID != 0) {
    localNodeId = NODE_ID;
    return;
  }

  const uint64_t mac = ESP.getEfuseMac();
  localNodeId = static_cast<uint32_t>((mac >> 32) ^ mac);
  if (localNodeId == 0) {
    localNodeId = 1;
  }
}

#if defined(ESP8266) || defined(ESP32)
IRAM_ATTR
#endif
void onRadioDio1() {
  radioRxFlag = true;
}

void scheduleNextTx(uint32_t nowMs) {
  const long jitter = random(-static_cast<long>(TX_JITTER_MS), static_cast<long>(TX_JITTER_MS) + 1L);
  nextTxMs = nowMs + TX_INTERVAL_MS + jitter;
}

int32_t degreesToE7(double value) {
  return static_cast<int32_t>(lround(value * 10000000.0));
}

uint16_t centiDegrees(double degrees) {
  return static_cast<uint16_t>(lround(normalizeDegrees(degrees) * 100.0)) % 36000;
}

uint16_t readBatteryMv() {
#if BATTERY_ADC_PIN >= 0
  #if BATTERY_ADC_ENABLE_PIN >= 0
    pinMode(BATTERY_ADC_ENABLE_PIN, OUTPUT);
    digitalWrite(BATTERY_ADC_ENABLE_PIN, BATTERY_ADC_ENABLE_ACTIVE_HIGH ? HIGH : LOW);
    delayMicroseconds(BATTERY_ADC_SETTLE_US);
  #endif

  uint32_t millivolts = 0;
  for (uint8_t sample = 0; sample < BATTERY_ADC_SAMPLES; ++sample) {
    millivolts += analogReadMilliVolts(BATTERY_ADC_PIN);
  }

  #if BATTERY_ADC_ENABLE_PIN >= 0
    digitalWrite(BATTERY_ADC_ENABLE_PIN, BATTERY_ADC_ENABLE_ACTIVE_HIGH ? LOW : HIGH);
  #endif
  const float inputMillivolts = static_cast<float>(millivolts) / BATTERY_ADC_SAMPLES;
  return static_cast<uint16_t>(lroundf(inputMillivolts * BATTERY_ADC_RATIO));
#else
  return 0;
#endif
}

uint8_t batteryPercentFromMv(uint16_t millivolts) {
  if (millivolts <= BATTERY_EMPTY_MV) {
    return 0;
  }
  if (millivolts >= BATTERY_FULL_MV) {
    return 100;
  }
  return static_cast<uint8_t>((static_cast<uint32_t>(millivolts - BATTERY_EMPTY_MV) * 100U) /
                              static_cast<uint32_t>(BATTERY_FULL_MV - BATTERY_EMPTY_MV));
}

bool readBatteryCharging() {
#if BATTERY_CHARGE_DETECT_PIN >= 0
  pinMode(BATTERY_CHARGE_DETECT_PIN, INPUT);
  return digitalRead(BATTERY_CHARGE_DETECT_PIN) == (BATTERY_CHARGE_DETECT_ACTIVE_LOW ? LOW : HIGH);
#else
  return false;
#endif
}

void updateBatteryStatus() {
  const uint16_t millivolts = readBatteryMv();
  localBattery.valid = millivolts >= BATTERY_EMPTY_MV - 200 && millivolts <= BATTERY_FULL_MV + 250;
  localBattery.millivolts = localBattery.valid ? millivolts : 0;
  localBattery.percent = localBattery.valid ? batteryPercentFromMv(millivolts) : 0;
  localBattery.charging = localBattery.valid && readBatteryCharging();
}

uint32_t colorForNode(uint32_t nodeId) {
  // Festival Tracker wearable identities: fixed colors are easier to recognize
  // than colors derived from an implementation-specific numeric ID.
  if (nodeId == 0x0000A101UL) { // HT-1
    return ring.Color(40, 255, 70);
  }
  if (nodeId == 0x0000A102UL) { // HT-2
    return ring.Color(45, 100, 255);
  }
  if (nodeId == 0x0000A103UL) { // HT-3
    return ring.Color(195, 45, 255);
  }

  switch (nodeId % 6) {
    case 0: return ring.Color(255, 40, 20);
    case 1: return ring.Color(30, 180, 255);
    case 2: return ring.Color(60, 255, 80);
    case 3: return ring.Color(255, 180, 20);
    case 4: return ring.Color(180, 80, 255);
    default: return ring.Color(255, 60, 180);
  }
}

uint32_t scaleColor(uint32_t color, uint8_t scale) {
  const uint8_t r = (color >> 16) & 0xFF;
  const uint8_t g = (color >> 8) & 0xFF;
  const uint8_t b = color & 0xFF;
  return ring.Color((static_cast<uint16_t>(r) * scale) / 255,
                    (static_cast<uint16_t>(g) * scale) / 255,
                    (static_cast<uint16_t>(b) * scale) / 255);
}

void addColorToLed(uint8_t led, uint32_t color) {
  const uint32_t existing = ring.getPixelColor(led);
  const uint8_t er = (existing >> 16) & 0xFF;
  const uint8_t eg = (existing >> 8) & 0xFF;
  const uint8_t eb = existing & 0xFF;
  const uint8_t r = (color >> 16) & 0xFF;
  const uint8_t g = (color >> 8) & 0xFF;
  const uint8_t b = color & 0xFF;
  ring.setPixelColor(led, ring.Color(min(255, er + r),
                                     min(255, eg + g),
                                     min(255, eb + b)));
}

void addColorToLedTarget(uint8_t led, uint32_t color) {
  const uint16_t r = (color >> 16) & 0xFF;
  const uint16_t g = (color >> 8) & 0xFF;
  const uint16_t b = color & 0xFF;
  const uint16_t nextR = ledTargetR[led] + r;
  const uint16_t nextG = ledTargetG[led] + g;
  const uint16_t nextB = ledTargetB[led] + b;
  ledTargetR[led] = nextR > 255 ? 255 : nextR;
  ledTargetG[led] = nextG > 255 ? 255 : nextG;
  ledTargetB[led] = nextB > 255 ? 255 : nextB;
}

void queuePeerLedCandidate(uint8_t led, uint32_t color) {
  if (led >= LED_COUNT || peerLedCandidateCount >= MAX_PEERS) {
    return;
  }
  peerLedCandidates[peerLedCandidateCount++] = PeerLedCandidate{led, color};
}

void renderSelectedPeerLed(uint32_t nowMs, uint8_t northLed, bool showNorthReference) {
  bool selectedPeer = false;
  uint8_t selectedLed = 0;
  if (peerLedCandidateCount > 0) {
    const uint32_t cycleMs = PEER_DISPLAY_HOLD_MS + PEER_DISPLAY_FADE_MS + PEER_DISPLAY_GAP_MS;
    const uint32_t safeCycleMs = cycleMs > 0 ? cycleMs : 1;
    const uint32_t cyclePositionMs = nowMs % safeCycleMs;
    const uint8_t selected = static_cast<uint8_t>((nowMs / safeCycleMs) % peerLedCandidateCount);
    const PeerLedCandidate &candidate = peerLedCandidates[selected];
    selectedPeer = true;
    selectedLed = candidate.led;
    uint8_t sequenceScale = 0;
    if (cyclePositionMs < PEER_DISPLAY_HOLD_MS) {
      sequenceScale = 255;
    } else if (cyclePositionMs < PEER_DISPLAY_HOLD_MS + PEER_DISPLAY_FADE_MS) {
      const uint32_t fadePositionMs = cyclePositionMs - PEER_DISPLAY_HOLD_MS;
      sequenceScale = static_cast<uint8_t>(255U - (fadePositionMs * 255U) / PEER_DISPLAY_FADE_MS);
    }
    if (sequenceScale > 0) {
      addColorToLedTarget(candidate.led, scaleColor(candidate.color, sequenceScale));
    }
  }

  // North is an orientation reference, not a peer. Keep it unless it would
  // overlap the single peer slot selected for this frame.
  if (showNorthReference && (!selectedPeer || selectedLed != northLed)) {
    addColorToLedTarget(northLed, ring.Color(24, 0, 0));
  }
}

float animateLedChannel(float current, uint16_t target) {
  // A new direction eases in over roughly four times the prior ramp duration.
  // Its predecessor fades over the same interval, producing a clean handoff.
  constexpr float riseFraction = 0.05f;
  constexpr float decayFraction = 0.4f;
  const float fraction = target > current ? riseFraction : decayFraction;
  return current + (static_cast<float>(target) - current) * fraction;
}

void renderLedTargets() {
  for (uint8_t led = 0; led < LED_COUNT; ++led) {
    ledShownR[led] = animateLedChannel(ledShownR[led], ledTargetR[led]);
    ledShownG[led] = animateLedChannel(ledShownG[led], ledTargetG[led]);
    ledShownB[led] = animateLedChannel(ledShownB[led], ledTargetB[led]);
    ring.setPixelColor(led, ring.Color(static_cast<uint8_t>(lroundf(ledShownR[led])),
                                       static_cast<uint8_t>(lroundf(ledShownG[led])),
                                       static_cast<uint8_t>(lroundf(ledShownB[led]))));
  }
  ring.show();
}

void runBootAnimation() {
  const uint32_t activeColor = colorForNode(localNodeId);
  uint8_t trail[LED_COUNT] = {};

  ring.clear();
  ring.show();
  for (uint8_t led = 0; led < LED_COUNT; ++led) {
    for (uint8_t previous = 0; previous < LED_COUNT; ++previous) {
      trail[previous] = (static_cast<uint16_t>(trail[previous]) * 3) / 4;
    }
    trail[led] = 255;

    ring.clear();
    for (uint8_t previous = 0; previous <= led; ++previous) {
      if (trail[previous] > 0) {
        ring.setPixelColor(previous, scaleColor(activeColor, trail[previous]));
      }
    }
    ring.show();
    delay(60);
  }
  ring.clear();
  ring.show();
}

void powerGnss() {
#if GNSS_POWER_PIN >= 0
  pinMode(GNSS_POWER_PIN, OUTPUT);
  digitalWrite(GNSS_POWER_PIN, GNSS_POWER_ACTIVE_HIGH ? HIGH : LOW);
#endif

#if GNSS_RESET_PIN >= 0
  pinMode(GNSS_RESET_PIN, OUTPUT);
  digitalWrite(GNSS_RESET_PIN, GNSS_RESET_ACTIVE_LOW ? LOW : HIGH);
  delay(120);
  digitalWrite(GNSS_RESET_PIN, GNSS_RESET_ACTIVE_LOW ? HIGH : LOW);
#endif

#if GNSS_WAKE_PIN >= 0
  pinMode(GNSS_WAKE_PIN, OUTPUT);
  digitalWrite(GNSS_WAKE_PIN, HIGH);
#endif

#if GNSS_PPS_PIN >= 0
  pinMode(GNSS_PPS_PIN, INPUT);
#endif

  // The Heltec V4 board variant waits for this rail and the L76K to settle
  // before opening the UART. Without it early boot data can be lost.
  delay(GNSS_WARMUP_MS);
}

void beginGnssUart(int rxPin) {
  Serial1.end();
  delay(20);
  Serial1.setRxBufferSize(GNSS_UART_RX_BUFFER_BYTES);

  // The app only consumes NMEA. Do not drive the module UART while probing
  // its receive line, which keeps the fallback electrically safe.
  Serial1.begin(GNSS_BAUD, SERIAL_8N1, rxPin, -1);
  activeGnssRxPin = rxPin;
  gnssUartStartedMs = millis();
}

void probeGnssUart(uint32_t nowMs) {
  if (gnssDataSeen || gnssFallbackRxActive ||
      static_cast<int32_t>(nowMs - gnssUartStartedMs) < static_cast<int32_t>(GNSS_UART_PROBE_MS)) {
    return;
  }

  Serial.println(F("GNSS no data on primary RX; trying fallback RX"));
  beginGnssUart(GNSS_UART_RX_FALLBACK_PIN);
  gnssFallbackRxActive = true;
}

void configureLoRaFrontEnd() {
  pinMode(LORA_FEM_POWER_PIN, OUTPUT);
  digitalWrite(LORA_FEM_POWER_PIN, HIGH);
  delay(5);

  // GPIO2 distinguishes the two documented Heltec V4 RF front-end revisions.
  pinMode(LORA_FEM_ENABLE_PIN, INPUT);
  delay(1);
  if (digitalRead(LORA_FEM_ENABLE_PIN) == HIGH) {
    loRaFemType = LoRaFemType::KCT8103L;
    pinMode(LORA_FEM_ENABLE_PIN, OUTPUT);
    digitalWrite(LORA_FEM_ENABLE_PIN, HIGH);
    pinMode(LORA_FEM_KCT8103L_RXTX_PIN, OUTPUT);
    digitalWrite(LORA_FEM_KCT8103L_RXTX_PIN, LOW);
    Serial.println(F("LoRa FEM: KCT8103L"));
  } else {
    loRaFemType = LoRaFemType::GC1109;
    pinMode(LORA_FEM_ENABLE_PIN, OUTPUT);
    digitalWrite(LORA_FEM_ENABLE_PIN, HIGH);
    pinMode(LORA_FEM_GC1109_TX_PIN, OUTPUT);
    digitalWrite(LORA_FEM_GC1109_TX_PIN, LOW);
    Serial.println(F("LoRa FEM: GC1109"));
  }
}

void setLoRaReceivePath() {
  if (loRaFemType == LoRaFemType::KCT8103L) {
    digitalWrite(LORA_FEM_POWER_PIN, HIGH);
    digitalWrite(LORA_FEM_ENABLE_PIN, HIGH);
    digitalWrite(LORA_FEM_KCT8103L_RXTX_PIN, LOW);
  } else if (loRaFemType == LoRaFemType::GC1109) {
    digitalWrite(LORA_FEM_ENABLE_PIN, HIGH);
    digitalWrite(LORA_FEM_GC1109_TX_PIN, LOW);
  }
}

void setLoRaTransmitPath() {
  if (loRaFemType == LoRaFemType::KCT8103L) {
    digitalWrite(LORA_FEM_POWER_PIN, HIGH);
    digitalWrite(LORA_FEM_ENABLE_PIN, HIGH);
    digitalWrite(LORA_FEM_KCT8103L_RXTX_PIN, HIGH);
  } else if (loRaFemType == LoRaFemType::GC1109) {
    digitalWrite(LORA_FEM_ENABLE_PIN, HIGH);
    digitalWrite(LORA_FEM_GC1109_TX_PIN, HIGH);
  }
}

void beginRadio() {
  configureLoRaFrontEnd();
  SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_NSS_PIN);

  int state = radio.begin(LORA_FREQ_MHZ,
                          LORA_BANDWIDTH_KHZ,
                          LORA_SPREADING_FACTOR,
                          LORA_CODING_RATE,
                          LORA_SYNC_WORD,
                          LORA_TX_POWER_DBM,
                          LORA_PREAMBLE_LEN,
                          LORA_TCXO_VOLTAGE);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Radio begin failed: "));
    Serial.println(state);
    return;
  }

  state = radio.setDio2AsRfSwitch(true);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Radio RF switch setup failed: "));
    Serial.println(state);
    return;
  }
  radio.setCRC(true);
  radio.setDio1Action(onRadioDio1);
  setLoRaReceivePath();
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Radio RX failed: "));
    Serial.println(state);
    return;
  }

  radioReady = true;
  Serial.println(F("SX1262 ready: FEM, DIO2 RF switch, DIO3 TCXO 1.8V"));
}

void beginCompass() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println(F("I2C compass bus started on SDA3/SCL4"));

#if USE_BNO055_COMPASS
  compassReady = bno.begin();
  if (compassReady) {
    bno.setExtCrystalUse(true);
  } else {
    Serial.println(F("BNO055 not found; falling back to GPS course while moving"));
  }
#else
  compassReady = false;
#endif
}

void scanI2cBus(TwoWire &bus, const __FlashStringHelper *label) {
  uint8_t found = 0;
  Serial.print(label);
  Serial.print(F(" scan:"));
  for (uint8_t address = 1; address < 127; ++address) {
    bus.beginTransmission(address);
    if (bus.endTransmission() == 0) {
      Serial.print(F(" 0x"));
      if (address < 16) {
        Serial.print('0');
      }
      Serial.print(address, HEX);
      ++found;
    }
  }
  if (found == 0) {
    Serial.print(F(" none"));
  }
  Serial.println();
}

void beginDisplay() {
#if DISPLAY_POWER_PIN >= 0
  pinMode(DISPLAY_POWER_PIN, OUTPUT);
  digitalWrite(DISPLAY_POWER_PIN, DISPLAY_POWER_ACTIVE_HIGH ? HIGH : LOW);
  delay(20);
#endif
  pinMode(DISPLAY_RESET_PIN, OUTPUT);
  digitalWrite(DISPLAY_RESET_PIN, HIGH);
  delay(20);
  displayWire.begin(DISPLAY_SDA_PIN, DISPLAY_SCL_PIN, 400000);
  Serial.println(F("I2C OLED bus started on SDA17/SCL18"));
  scanI2cBus(displayWire, F("OLED"));
  displayReady = display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_I2C_ADDRESS);
  if (!displayReady) {
    Serial.println(F("OLED display not found"));
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("LoRa compass mesh"));
  display.display();
}

void updateGnss() {
  while (Serial1.available() > 0) {
    gnssDataSeen = true;
    ++gnssBytesRead;
    gps.encode(static_cast<char>(Serial1.read()));
  }
}

void updateHeading() {
  bool hasHeading = false;
  double measuredHeadingDeg = currentHeadingDeg;
#if USE_BNO055_COMPASS
  if (compassReady) {
    sensors_event_t event;
    bno.getEvent(&event);
    if (!isnan(event.orientation.x)) {
      measuredHeadingDeg = normalizeDegrees(event.orientation.x);
      hasHeading = true;
    }
  }
#endif

  if (!hasHeading && gps.course.isValid() && gps.speed.kmph() > 2.0) {
    measuredHeadingDeg = normalizeDegrees(gps.course.deg());
    hasHeading = true;
  }

  if (hasHeading) {
    currentHeadingDeg = headingFilter.update(measuredHeadingDeg, HEADING_FILTER_ALPHA);
  }
}

bool haveValidLocation() {
  return gps.location.isValid() && gps.location.age() < 5000;
}

GeoPoint currentLocation() {
  return GeoPoint{gps.location.lat(), gps.location.lng()};
}

double currentAltitudeM() {
  return gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
}

void updateNavigationPosition(uint32_t nowMs) {
  if (!haveValidLocation()) {
    return;
  }

  // TinyGPSPlus marks a location update once per newly parsed GNSS fix. Do not
  // feed the same sample into the filter on every main-loop iteration.
  if (navigationFrame.isValid() && !gps.location.isUpdated()) {
    return;
  }

  const GeoPoint rawPosition = currentLocation();
  const double altitudeM = currentAltitudeM();
  if (!navigationFrame.isValid()) {
    navigationFrame.setOrigin(rawPosition, altitudeM);
    localPositionFilter.reset(EnuPoint{0.0, 0.0, 0.0}, nowMs);
  } else {
    localPositionFilter.update(navigationFrame.toEnu(rawPosition, altitudeM), nowMs, positionFilterConfig);
  }
  peers.initializePositions(navigationFrame, nowMs);
}

bool haveNavigationPosition() {
  return navigationFrame.isValid() && localPositionFilter.isValid();
}

double uncertaintyMeters(uint16_t hdopCenti) {
  if (hdopCenti == 0) {
    return GNSS_UNKNOWN_UNCERTAINTY_M;
  }
  const double hdop = static_cast<double>(hdopCenti) / 100.0;
  const double estimated = hdop * GNSS_HDOP_TO_UNCERTAINTY_M;
  return estimated > GNSS_MIN_UNCERTAINTY_M ? estimated : GNSS_MIN_UNCERTAINTY_M;
}

double localUncertaintyMeters() {
  return gps.hdop.isValid() ? uncertaintyMeters(static_cast<uint16_t>(gps.hdop.value()))
                            : GNSS_UNKNOWN_UNCERTAINTY_M;
}

MeshPacketV2 buildPacket() {
  MeshPacketV2 packet = {};
  packet.nodeId = localNodeId;
  packet.sequence = txSequence++;
  packet.uptimeMs = millis();
  packet.latE7 = haveValidLocation() ? degreesToE7(gps.location.lat()) : 0;
  packet.lonE7 = haveValidLocation() ? degreesToE7(gps.location.lng()) : 0;
  packet.altM = gps.altitude.isValid() ? static_cast<int16_t>(lround(gps.altitude.meters())) : 0;
  packet.speedCentiKph = gps.speed.isValid() ? static_cast<uint16_t>(lround(gps.speed.kmph() * 100.0)) : 0;
  packet.courseCdeg = gps.course.isValid() ? centiDegrees(gps.course.deg()) : 0;
  packet.headingCdeg = centiDegrees(currentHeadingDeg);
  packet.hdopCenti = gps.hdop.isValid() ? static_cast<uint16_t>(gps.hdop.value()) : 0;
  packet.sats = gps.satellites.isValid() ? static_cast<uint8_t>(gps.satellites.value()) : 0;
  packet.batteryMv = localBattery.valid ? localBattery.millivolts : 0;
  packet.flags = 0;
  if (haveValidLocation()) {
    packet.flags |= FLAG_GNSS_VALID;
  }
  if (compassReady || (gps.course.isValid() && gps.speed.kmph() > 2.0)) {
    packet.flags |= FLAG_COMPASS_VALID;
  }
  if (packet.batteryMv > 0 && packet.batteryMv < BATTERY_LOW_MV) {
    packet.flags |= FLAG_LOW_BATTERY;
  }
  packet.hopLimit = MESH_DEFAULT_HOP_LIMIT;
  packet.hopCount = 0;

  finalizePacket(packet);
  return packet;
}

void transmitPosition(uint32_t nowMs) {
  if (!radioReady) {
    return;
  }

  MeshPacketV2 packet = buildPacket();
  setLoRaTransmitPath();
  radio.standby();
  const int state = radio.transmit(reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("TX failed: "));
    Serial.println(state);
  } else {
    ++radioTxCount;
  }
  Serial.print(F("TX origin=0x"));
  Serial.print(localNodeId, HEX);
  Serial.print(F(" seq="));
  Serial.println(packet.sequence);
  setLoRaReceivePath();
  radio.startReceive();
  scheduleNextTx(nowMs);
}

void queueForward(const MeshPacketV2 &received, uint32_t nowMs) {
  if (received.hopLimit == 0 || forwardQueueCount >= MAX_FORWARD_QUEUE) {
    return;
  }

  MeshPacketV2 forwarded = received;
  --forwarded.hopLimit;
  ++forwarded.hopCount;
  finalizePacket(forwarded);

  if (forwardQueueCount == 0) {
    nextForwardMs = nowMs + random(FORWARD_DELAY_MIN_MS, FORWARD_DELAY_MAX_MS + 1UL);
  }
  forwardQueue[forwardQueueTail] = forwarded;
  forwardQueueTail = (forwardQueueTail + 1) % MAX_FORWARD_QUEUE;
  ++forwardQueueCount;
}

void handleForward(uint32_t nowMs) {
  if (!radioReady || forwardQueueCount == 0 || static_cast<int32_t>(nowMs - nextForwardMs) < 0) {
    return;
  }

  MeshPacketV2 packet = forwardQueue[forwardQueueHead];
  forwardQueueHead = (forwardQueueHead + 1) % MAX_FORWARD_QUEUE;
  --forwardQueueCount;

  setLoRaTransmitPath();
  radio.standby();
  const int state = radio.transmit(reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Forward failed: "));
    Serial.println(state);
  } else {
    ++radioTxCount;
    Serial.print(F("Forwarded origin=0x"));
    Serial.print(packet.nodeId, HEX);
    Serial.print(F(" seq="));
    Serial.print(packet.sequence);
    Serial.print(F(" hops="));
    Serial.println(packet.hopCount);
  }
  setLoRaReceivePath();
  radio.startReceive();
  nextForwardMs = nowMs + random(FORWARD_DELAY_MIN_MS, FORWARD_DELAY_MAX_MS + 1UL);
}

void handleRadioRx(uint32_t nowMs) {
  if (!radioReady || !radioRxFlag) {
    return;
  }

  radioRxFlag = false;
  const int state = radio.readData(rxBuffer, sizeof(rxBuffer));
  if (state == RADIOLIB_ERR_NONE) {
    MeshPacketV2 packet = {};
    memcpy(&packet, rxBuffer, sizeof(packet));
    if (packet.nodeId != localNodeId && packetIsValid(packet)) {
      const PeerUpdateResult result = peers.upsert(packet,
                                                   static_cast<int16_t>(radio.getRSSI()),
                                                   radio.getSNR(),
                                                   nowMs,
                                                   navigationFrame,
                                                   positionFilterConfig);
      if (result != PeerUpdateResult::Accepted) {
        Serial.print(F("RX rejected origin=0x"));
        Serial.print(packet.nodeId, HEX);
        Serial.print(F(" reason="));
        if (result == PeerUpdateResult::UnrealisticJump) {
          Serial.println(F("jump"));
        } else if (result == PeerUpdateResult::Stale) {
          Serial.println(F("stale"));
        } else {
          Serial.println(F("sequence"));
        }
      } else {
      ++radioRxCount;
      Serial.print(F("RX origin=0x"));
      Serial.print(packet.nodeId, HEX);
      Serial.print(F(" seq="));
      Serial.print(packet.sequence);
      Serial.print(F(" hops="));
      Serial.println(packet.hopCount);
      queueForward(packet, nowMs);
      }
    }
  }
  setLoRaReceivePath();
  radio.startReceive();
}

void renderPeers(uint32_t nowMs) {
  memset(ledTargetR, 0, sizeof(ledTargetR));
  memset(ledTargetG, 0, sizeof(ledTargetG));
  memset(ledTargetB, 0, sizeof(ledTargetB));
  peerLedCandidateCount = 0;

  uint8_t northLed = 0;
#if SHOW_NORTH_REFERENCE
  northLed = ledIndexForRelativeBearing(shortestAngleDelta(currentHeadingDeg, 0.0), LED_COUNT);
#endif

  if (!haveNavigationPosition()) {
#if SHOW_NORTH_REFERENCE
    addColorToLedTarget(northLed, ring.Color(24, 0, 0));
#endif
    const uint8_t pulse = static_cast<uint8_t>((sin(nowMs / 220.0) + 1.0) * 18.0);
    addColorToLedTarget(0, scaleColor(colorForNode(localNodeId), pulse));
    renderLedTargets();
    return;
  }

  const EnuPoint here = localPositionFilter.position();
  const double localUncertainty = localUncertaintyMeters();
  for (Peer &peer : peers) {
    if (!peer.active || nowMs - peer.lastSeenMs > PEER_TIMEOUT_MS || !(peer.flags & FLAG_GNSS_VALID) ||
        !peer.positionFilter.isValid()) {
      continue;
    }

    const EnuPoint there = peer.positionFilter.position();
    const double meters = enuDistanceMeters(here, there);
    const double rawBearing = enuBearingDegrees(here, there);
    const double combinedUncertainty = hypot(localUncertainty, uncertaintyMeters(peer.hdopCenti));
    // An absent/poor HDOP must not turn a 15-20 m neighbour into a false
    // close-proximity indication. Use uncertainty to expand the 8 m minimum,
    // but cap that expansion at the explicitly configured close range.
    const double uncertaintyTriggerMeters = combinedUncertainty < CLOSE_RANGE_MAX_TRIGGER_M
                                                ? combinedUncertainty
                                                : CLOSE_RANGE_MAX_TRIGGER_M;
    const double nearbyEnterMeters = uncertaintyTriggerMeters > CLOSE_RANGE_THRESHOLD_M
                                         ? uncertaintyTriggerMeters
                                         : CLOSE_RANGE_THRESHOLD_M;
    const double nearbyExitMeters = nearbyEnterMeters + CLOSE_RANGE_EXIT_HYSTERESIS_M;
    if (!peer.nearbyUncertain && meters <= nearbyEnterMeters) {
      peer.heldBearingDeg = peer.bearingFilter.update(rawBearing, BEARING_FILTER_ALPHA);
      peer.nearbyUncertain = true;
      peer.nearbySinceMs = nowMs;
      Serial.print(F("NEAR enter peer=0x"));
      Serial.print(peer.nodeId, HEX);
      Serial.print(F(" dist_m="));
      Serial.print(meters, 1);
      Serial.print(F(" trigger_m="));
      Serial.print(nearbyEnterMeters, 1);
      Serial.print(F(" uncertainty_m="));
      Serial.println(combinedUncertainty, 1);
    } else if (peer.nearbyUncertain && meters > nearbyExitMeters) {
      peer.nearbyUncertain = false;
      peer.bearingFilter.reset(rawBearing);
      peer.directionLedValid = false;
      Serial.print(F("NEAR exit peer=0x"));
      Serial.print(peer.nodeId, HEX);
      Serial.print(F(" dist_m="));
      Serial.println(meters, 1);
    }

    const uint8_t freshness = static_cast<uint8_t>(constrain(255 - ((nowMs - peer.lastSeenMs) * 180 / PEER_TIMEOUT_MS), 40, 255));
    const uint8_t distanceScale = static_cast<uint8_t>(constrain(255 - min(180.0, meters / 8.0), 70.0, 255.0));
    const uint8_t scale = min(freshness, distanceScale);
    const uint32_t peerColor = colorForNode(peer.nodeId);

    if (peer.nearbyUncertain) {
      // Nearby peers remain a single held direction. Only their blink rate
      // changes; proximity must never light the whole ring.
      const double relativeBearing = shortestAngleDelta(currentHeadingDeg, peer.heldBearingDeg);
      const uint8_t led = ledIndexWithHysteresis(relativeBearing,
                                                  peer.directionLed,
                                                  peer.directionLedValid,
                                                  LED_COUNT,
                                                  LED_DIRECTION_HYSTERESIS_DEG);
      peer.directionLed = led;
      peer.directionLedValid = true;
      queuePeerLedCandidate(led, scaleColor(peerColor, scale));
      continue;
    }

    const double absoluteBearing = peer.bearingFilter.update(rawBearing, BEARING_FILTER_ALPHA);
    const double relativeBearing = shortestAngleDelta(currentHeadingDeg, absoluteBearing);
    const uint8_t led = ledIndexWithHysteresis(relativeBearing,
                                                peer.directionLed,
                                                peer.directionLedValid,
                                                LED_COUNT,
                                                LED_DIRECTION_HYSTERESIS_DEG);
    peer.directionLed = led;
    peer.directionLedValid = true;
    queuePeerLedCandidate(led, scaleColor(peerColor, scale));
  }

  renderSelectedPeerLed(nowMs, northLed, SHOW_NORTH_REFERENCE != 0);
  renderLedTargets();
}

void printStatus(uint32_t nowMs) {
  Serial.print(F("node=0x"));
  Serial.print(localNodeId, HEX);
  Serial.print(F(" fix="));
  Serial.print(haveValidLocation() ? F("yes") : F("no"));
  Serial.print(F(" sats="));
  Serial.print(gps.satellites.isValid() ? gps.satellites.value() : 0);
  Serial.print(F(" gnss="));
  Serial.print(gnssDataSeen ? F("data") : F("none"));
  Serial.print(F(" bytes="));
  Serial.print(gnssBytesRead);
  Serial.print(F(" parsed="));
  Serial.print(gps.charsProcessed());
  Serial.print(F(" heading="));
  Serial.print(currentHeadingDeg, 1);
  Serial.print(F(" battery="));
  if (localBattery.valid) {
    Serial.print(localBattery.millivolts);
    Serial.print(F("mV/"));
    Serial.print(localBattery.percent);
    Serial.print('%');
    if (localBattery.charging) {
      Serial.print(F(" charging"));
    }
  } else {
    Serial.print(F("unknown"));
  }
  Serial.print(F(" peers="));
  Serial.print(peers.countActive(nowMs));
  Serial.print(F(" radio="));
  Serial.println(radioReady ? F("ok") : F("fail"));

  if (haveNavigationPosition()) {
    const EnuPoint here = localPositionFilter.position();
    for (const Peer &peer : peers) {
      if (!peer.active || nowMs - peer.lastSeenMs > PEER_TIMEOUT_MS || !(peer.flags & FLAG_GNSS_VALID) ||
          !peer.positionFilter.isValid()) {
        continue;
      }

      const EnuPoint there = peer.positionFilter.position();
      const double bearing = enuBearingDegrees(here, there);
      const double relative = shortestAngleDelta(currentHeadingDeg, bearing);
      Serial.print(F("  peer=0x"));
      Serial.print(peer.nodeId, HEX);
      Serial.print(F(" dist_m="));
      Serial.print(enuDistanceMeters(here, there), 1);
      Serial.print(F(" bearing="));
      Serial.print(bearing, 1);
      Serial.print(F(" rel="));
      Serial.print(relative, 1);
      if (peer.nearbyUncertain) {
        Serial.print(F(" near=uncertain"));
      }
      Serial.print(F(" rssi="));
      Serial.print(peer.rssiDbm);
      Serial.print(F(" snr="));
      Serial.print(peer.snrDb, 1);
      Serial.print(F(" hops="));
      Serial.println(peer.hopCount);
    }
  }
}

void drawBatteryIndicator() {
  constexpr int16_t batteryTextX = 72;
  constexpr int16_t batteryIconX = 110;
  constexpr int16_t batteryIconY = 0;
  constexpr int16_t batteryIconWidth = 15;
  constexpr int16_t batteryIconHeight = 7;

  display.setCursor(batteryTextX, 0);
  display.print(F("B"));
  if (localBattery.valid) {
    display.print(localBattery.percent);
    display.print('%');
  } else {
    display.print(F("--"));
  }

  display.drawRect(batteryIconX, batteryIconY, batteryIconWidth, batteryIconHeight, SSD1306_WHITE);
  display.fillRect(batteryIconX + batteryIconWidth, batteryIconY + 2, 2, 3, SSD1306_WHITE);
  if (!localBattery.valid) {
    display.drawLine(batteryIconX + 2, batteryIconY + 1,
                     batteryIconX + batteryIconWidth - 3, batteryIconY + batteryIconHeight - 2,
                     SSD1306_WHITE);
    return;
  }

  const int16_t fillWidth = 1 + ((batteryIconWidth - 4) * localBattery.percent) / 100;
  display.fillRect(batteryIconX + 2, batteryIconY + 2, fillWidth, batteryIconHeight - 3, SSD1306_WHITE);
  if (localBattery.charging) {
    display.drawLine(batteryIconX + 8, batteryIconY + 1, batteryIconX + 6, batteryIconY + 4, SSD1306_BLACK);
    display.drawLine(batteryIconX + 6, batteryIconY + 4, batteryIconX + 9, batteryIconY + 4, SSD1306_BLACK);
    display.drawLine(batteryIconX + 9, batteryIconY + 4, batteryIconX + 7, batteryIconY + 6, SSD1306_BLACK);
  }
}

void renderDisplay(uint32_t nowMs) {
  if (!displayReady) {
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print(F("GNSS "));
  if (haveValidLocation()) {
    display.print(F("FIX "));
  } else if (gnssDataSeen) {
    display.print(F("WAIT "));
  } else {
    display.print(F("NONE "));
  }
  display.print(F(" R"));
  display.print(activeGnssRxPin);
  drawBatteryIndicator();

  display.setCursor(0, 8);
  display.print(F("LAT "));
  if (haveValidLocation()) {
    display.println(gps.location.lat(), 5);
  } else {
    display.println(F("--"));
  }

  display.setCursor(0, 16);
  display.print(F("LON "));
  if (haveValidLocation()) {
    display.println(gps.location.lng(), 5);
  } else {
    display.println(F("--"));
  }

  display.setCursor(0, 24);
  display.print(F("COMP "));
  if (compassReady) {
    display.print(F("BNO "));
    display.print(currentHeadingDeg, 1);
    display.println(F(" deg"));
  } else if (gps.course.isValid() && gps.speed.kmph() > 2.0) {
    display.print(F("GPS "));
    display.print(currentHeadingDeg, 1);
    display.println(F(" deg"));
  } else {
    display.println(F("NONE"));
  }

  display.setCursor(0, 32);
  display.print(F("LORA "));
  display.print(radioReady ? F("OK") : F("FAIL"));
  display.print(F(" T"));
  display.print(radioTxCount);
  display.print(F(" R"));
  display.print(radioRxCount);
  display.print(F(" P"));
  display.println(peers.countActive(nowMs));

  uint8_t shownPeers = 0;
  for (const Peer &peer : peers) {
    if (shownPeers >= 3 || !peer.active || nowMs - peer.lastSeenMs > PEER_TIMEOUT_MS || !haveNavigationPosition() ||
        !(peer.flags & FLAG_GNSS_VALID) || !peer.positionFilter.isValid()) {
      continue;
    }

    const EnuPoint here = localPositionFilter.position();
    const EnuPoint there = peer.positionFilter.position();
    const double distance = enuDistanceMeters(here, there);
    const double bearing = peer.nearbyUncertain ? peer.heldBearingDeg : enuBearingDegrees(here, there);
    display.setCursor(0, 40 + (shownPeers * 8));
    display.print(F("P "));
    display.print(peer.nodeId, HEX);
    display.print(F(" "));
    if (distance >= 1000.0) {
      display.print(distance / 1000.0, 1);
      display.print(F("km"));
    } else {
      display.print(distance, 0);
      display.print(F("m"));
    }
    display.print(F(" B"));
    if (peer.nearbyUncertain) {
      display.println(F(" NEAR"));
    } else {
      display.print(bearing, 0);
      display.println(F("\xB0"));
    }
    ++shownPeers;
  }

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println(F("Heltec V4 LoRa compass mesh boot"));

  randomSeed(esp_random());
  initializeNodeId();
  Serial.print(F("Mesh node ID 0x"));
  Serial.println(localNodeId, HEX);
  ring.begin();
  ring.setBrightness(LED_BRIGHTNESS);
  ring.clear();
  ring.show();
  Serial.print(F("LED ring GPIO "));
  Serial.println(LED_RING_PIN);
  runBootAnimation();

  powerGnss();
  Serial.println(F("GNSS power configured"));
  beginGnssUart(GNSS_UART_RX_PIN);
  Serial.print(F("GNSS UART RX="));
  Serial.print(GNSS_UART_RX_PIN);
  Serial.print(F(" TX=disabled"));
  Serial.print(F(" baud="));
  Serial.println(GNSS_BAUD);
  beginDisplay();
  beginCompass();
  scanI2cBus(Wire, F("COMPASS"));
  beginRadio();
  Serial.println(F("Initialization complete"));

  const uint32_t nowMs = millis();
  scheduleNextTx(nowMs + random(0, TX_INTERVAL_MS));
  nextLedMs = nowMs;
  nextStatusMs = nowMs + SERIAL_STATUS_MS;
  nextDisplayMs = nowMs;
  updateBatteryStatus();
  nextBatterySampleMs = nowMs + BATTERY_SAMPLE_MS;
}

void loop() {
  const uint32_t nowMs = millis();
  updateGnss();
  updateNavigationPosition(nowMs);
  if (static_cast<int32_t>(nowMs - nextBatterySampleMs) >= 0) {
    updateBatteryStatus();
    nextBatterySampleMs = nowMs + BATTERY_SAMPLE_MS;
  }
  probeGnssUart(nowMs);
  updateHeading();
  handleRadioRx(nowMs);
  peers.prune(nowMs);

  if (static_cast<int32_t>(nowMs - nextForwardMs) >= 0) {
    handleForward(nowMs);
  }

  if (static_cast<int32_t>(nowMs - nextTxMs) >= 0) {
    transmitPosition(nowMs);
  }

  if (static_cast<int32_t>(nowMs - nextLedMs) >= 0) {
    renderPeers(nowMs);
    nextLedMs = nowMs + LED_FRAME_MS;
  }

  if (static_cast<int32_t>(nowMs - nextStatusMs) >= 0) {
    printStatus(nowMs);
    nextStatusMs = nowMs + SERIAL_STATUS_MS;
  }

  if (static_cast<int32_t>(nowMs - nextDisplayMs) >= 0) {
    renderDisplay(nowMs);
    nextDisplayMs = nowMs + DISPLAY_UPDATE_MS;
  }
}
