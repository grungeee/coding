/*
  Standalone ESP32 38-pin potentiometer to PWM converter.

  Local analog inputs (index = board slot):
    board1/POT1 -> GPIO36
    board2/POT2 -> GPIO35
    board3/POT3 -> GPIO34
    Pots use ADC1 pins so they keep working while WiFi/ESP-NOW is active.

  I2C:
    Not assigned in this 38-pin core-functionality pinout.

  Switch inputs (wired outside the IDC connector; SW1/SW2 are crossed on the harness):
    switch1 in -> GPIO21
    switch2 in -> GPIO4
    switch3 in -> GPIO15
    Switch input is ON when connected to 3.3 V.
    INPUT_PULLDOWN is used, so LOW=released/off and HIGH=pressed/on.
    All three have an internal pull-down, so no external resistor is needed.

  IDC connector: 12 pins, 6 PWM + 6 digital outputs, split into 3 four-pin blocks.
  ESP32 right-header pins land on the odd IDC positions and left-header pins on the
  even positions, so the even row is the one that crosses over the soldered odd row.
  GPIO12 and GPIO14 are not used anywhere.

  Digital outputs (binary, one pair per board):
    board1/switch1 -> DO1a GPIO22 (IDC 3),  DO1b GPIO33 (IDC 4)
    board2/switch2 -> DO2a GPIO18 (IDC 7),  DO2b GPIO26 (IDC 8)
    board3/switch3 -> DO3a GPIO16 (IDC 11), DO3b GPIO13 (IDC 12)

  PWM outputs (one pair per board):
    board1/switch1 -> PWM1 GPIO23 (IDC 1), PWM2 GPIO32 (IDC 2)
    board2/switch2 -> PWM3 GPIO19 (IDC 5), PWM4 GPIO25 (IDC 6)
    board3/switch3 -> PWM5 GPIO17 (IDC 9), PWM6 GPIO27 (IDC 10)

  Serial output at 115200 baud prints ADC values and converted output voltages.

  Control paths (either one can drive the board):
    1. USB serial commands (115200), newline terminated:
         SET pot1|pot2|pot3 0..4095      force a pot input
         SET switch 0|1|on|off           force the switch state
         CLR pot1|pot2|pot3|switch|all   release pot/switch overrides
         PWM <1..6> <0..4095>            force one PWM channel
         PWM <1..6> off                  release one PWM channel
         DO <1..6> <0|1>                 force one digital output
         DO <1..6> off                   release one digital output
         PARAM                           list all parameters
         PARAM <name>                    show one parameter
         PARAM <name> <value>            set one parameter
         SAVE | LOAD | DEFAULTS          persist / restore / reset parameters
         RELEASE                         release every override and force
         STATUS                          report a summary
    2. ESP-NOW broadcast on channel 1 (the M5Stack Cardputer UI, or any sender
       that speaks the packet below). The Cardputer can do everything the serial
       port can, including per-signal force and parameter tuning.

  ESP-NOW packet (must stay byte-identical to m5stack_cardputer_pot2pwm_ui.ino):
    magic 0x50325057, version 2, sizeof == 150 bytes.
    kind 1 = telemetry (controller -> UI), kind 2 = command (UI -> controller).
*/

#include <WiFi.h>
#include <esp_now.h>
#include <Preferences.h>

/* The harness connectors are indexed by BOARD SLOT below (slot N = board N = physical
   POTN / SWN). Measured on the board:
     pots:  POT1 = GPIO36, POT2 = GPIO35, POT3 = GPIO34   (pot order IS reversed on the harness)
     switches: SW1 = GPIO21, SW2 = GPIO4, SW3 = GPIO15    (SW1/SW2 are crossed)
   The PWM and digital-output arrays follow the IDC connector order and are untouched. */
const int potPins[3] = {36, 35, 34};
const int digitalOutPins[6] = {22, 33, 18, 26, 16, 13};
const int switchDigitalOutPins[3][2] = {
  {22, 33},
  {18, 26},
  {16, 13}
};
const int pwmPins[6] = {23, 32, 19, 25, 17, 27};
const uint8_t boardPwmIndexes[3][2] = {
  {0, 1},
  {2, 3},
  {4, 5}
};
const int switchInputPins[3] = {21, 4, 15};

/* ---------------------------------------------------------------- parameters */

enum ParamId : uint8_t {
  PARAM_PWM_FREQ = 0,
  PARAM_PWM_RES,
  PARAM_POT_LOW_IN,
  PARAM_POT_LOW_DUTY,
  PARAM_POT_HIGH_IN,
  PARAM_POT_HIGH_DUTY,
  PARAM_MAX_B1,
  PARAM_MAX_B2,
  PARAM_MAX_B3,
  PARAM_RAMP_MS,
  PARAM_REPORT_MS,
  PARAM_MOVE_THRESH,
  PARAM_MOVE_HOLD_MS,
  PARAM_SWITCH_MAX,
  PARAM_INVERT_POT1,
  PARAM_INVERT_POT2,
  PARAM_INVERT_POT3,
  PARAM_POT_FLIP,
  PARAM_COUNT
};

struct ParamSpec {
  const char* name;
  uint16_t minValue;
  uint16_t maxValue;
  uint16_t step;
  uint16_t defaultValue;
};

const ParamSpec paramSpecs[PARAM_COUNT] = {
  {"freq",     100, 40000, 500, 15000},
  {"res",        8,    14,   1,    12},
  {"lowin",      0,  4095,  32,   100},
  {"lowduty",    0,  4095,  32,   559},
  {"highin",     0,  4095,  32,  4095},
  {"highduty",   0,  4095,  32,  2047},
  {"maxb1",      0,  4095,  64,  2047},
  {"maxb2",      0,  4095,  64,  2047},
  {"maxb3",      0,  4095,  64,  2047},
  {"rampms",     0, 60000, 250,  5000},
  {"reportms",  20,  2000,  10,   100},
  {"movethr",    0,  1000,   4,    16},
  {"movehold",   0,  5000,  50,   500},
  {"swmax",      0,  4095,  64,  4095},
  {"inv1",       0,     1,   1,     0},
  {"inv2",       0,     1,   1,     0},
  {"inv3",       0,     1,   1,     0},
  {"potflip",    0,     1,   1,     0},
};

uint16_t paramValues[PARAM_COUNT];
bool paramsFromNvs = false;

const int adcMax = 4095;
const float fullScaleVoltage = 3.3f;
const unsigned long movementSampleMs = 50;
const uint8_t telemetryHeaderEvery = 20;
const size_t commandMaxLength = 64;
const uint32_t paramBlobMagic = 0x50325032UL;

/* ------------------------------------------------------------------ protocol */

const uint8_t espNowChannel = 1;
const uint32_t packetMagic = 0x50325057;
const uint8_t packetVersion = 2;
const uint8_t packetKindTelemetry = 1;
const uint8_t packetKindCommand = 2;
const uint8_t commandSet = 1;
const uint8_t commandClear = 2;
const uint8_t commandForcePwm = 3;
const uint8_t commandForceDo = 4;
const uint8_t commandReleasePwm = 5;
const uint8_t commandReleaseDo = 6;
const uint8_t commandSetParam = 7;
const uint8_t commandSaveParams = 8;
const uint8_t commandLoadParams = 9;
const uint8_t commandResetParams = 10;
const uint8_t commandReleaseAll = 11;
const uint8_t variablePot1 = 1;
const uint8_t variablePot2 = 2;
const uint8_t variablePot3 = 3;
const uint8_t variableSwitch = 4;
const uint8_t variableAll = 255;
const uint8_t broadcastAddress[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct EspNowPacket {
  uint32_t magic;
  uint8_t version;
  uint8_t kind;
  uint16_t seq;
  uint8_t command;
  uint8_t variable;
  uint16_t commandValue;
  uint16_t potValue[3];
  uint16_t pwmDuty[6];
  uint16_t switchDuty;
  uint8_t flags;
  uint8_t pwmForceMask;
  uint8_t doForceMask;
  uint8_t doStateMask;
  uint8_t switchRawMask;
  uint16_t params[PARAM_COUNT];
  uint16_t paramMin[PARAM_COUNT];
  uint16_t paramMax[PARAM_COUNT];
  uint16_t paramStep[PARAM_COUNT];
  uint8_t paramCount;
} __attribute__((packed));

static_assert(sizeof(EspNowPacket) == 182, "ESP-NOW packet must match m5stack_cardputer_pot2pwm_ui.ino");

struct ParamBlob {
  uint32_t magic;
  uint16_t values[PARAM_COUNT];
};

/* ------------------------------------------------------------------- runtime */

unsigned long lastReportMs = 0;
unsigned long startupMs = 0;
unsigned long switchRampStartMs = 0;
unsigned long lastMovementSampleMs = 0;
unsigned long lastPwm23ChangeMs = 0;
uint16_t sequence = 0;
uint16_t lastPot2 = 0;
uint16_t lastPot3 = 0;
bool haveLastPots = false;
bool lastSwitchOn = false;
uint32_t dutyFullScale = 4095;
String usbCommandLine;
EspNowPacket pendingCommandPacket;
volatile bool havePendingCommandPacket = false;
portMUX_TYPE pendingCommandMux = portMUX_INITIALIZER_UNLOCKED;
bool pwmAttachOk[6] = {false, false, false, false, false, false};
Preferences preferences;

struct AdcOverride {
  bool enabled = false;
  uint16_t value = 0;
};

struct BoolOverride {
  bool enabled = false;
  bool value = false;
};

struct PwmForce {
  bool enabled = false;
  uint16_t value = 0;
};

struct DoForce {
  bool enabled = false;
  bool value = false;
};

AdcOverride potOverrides[3];
BoolOverride switchOverride;
PwmForce pwmForce[6];
DoForce doForce[6];

/* ------------------------------------------------------------------- helpers */

float outputVoltage(uint16_t value) {
  return (value * fullScaleVoltage) / adcMax;
}

float dutyPercent(uint16_t duty) {
  return (duty * 100.0f) / adcMax;
}

const char* sourceName(bool overridden) {
  return overridden ? "MAN" : "AUTO";
}

const char* channelSourceName(uint8_t index) {
  if (pwmForce[index].enabled) {
    return "FORCE";
  }

  return sourceName(potOverrides[index / 2].enabled);
}

uint16_t clampAdc(long value) {
  if (value < 0) {
    return 0;
  }

  if (value > adcMax) {
    return adcMax;
  }

  return (uint16_t)value;
}

uint16_t cappedDuty(uint16_t value, uint16_t maxValue) {
  return value > maxValue ? maxValue : value;
}

uint32_t scaleDuty(uint16_t duty) {
  return ((uint32_t)duty * dutyFullScale + (adcMax / 2)) / adcMax;
}

uint16_t sensitiveDuty(uint16_t value) {
  uint16_t lowIn = paramValues[PARAM_POT_LOW_IN];
  uint16_t lowDuty = paramValues[PARAM_POT_LOW_DUTY];
  uint16_t highIn = paramValues[PARAM_POT_HIGH_IN];
  uint16_t highDuty = paramValues[PARAM_POT_HIGH_DUTY];

  if (value <= lowIn) {
    if (lowIn == 0) {
      return 0;
    }

    return ((uint32_t)value * lowDuty + (lowIn / 2)) / lowIn;
  }

  if (highIn <= lowIn) {
    return highDuty > adcMax ? adcMax : highDuty;
  }

  uint32_t inputSpan = highIn - lowIn;
  uint32_t dutySpan = (highDuty > lowDuty) ? (uint32_t)(highDuty - lowDuty) : 0;
  uint32_t scaled = lowDuty + (((uint32_t)(value - lowIn) * dutySpan + (inputSpan / 2)) / inputSpan);
  return scaled > adcMax ? adcMax : (uint16_t)scaled;
}

uint16_t startupRampedDuty(uint16_t value, unsigned long nowMs) {
  uint16_t rampMs = paramValues[PARAM_RAMP_MS];

  if (rampMs == 0) {
    return value;
  }

  unsigned long elapsedMs = nowMs - startupMs;

  if (elapsedMs >= rampMs) {
    return value;
  }

  uint32_t rampMax = ((uint32_t)adcMax * elapsedMs) / rampMs;
  return value > rampMax ? rampMax : value;
}

uint16_t switchRampedDuty(bool switchOn, unsigned long nowMs) {
  if (!switchOn) {
    lastSwitchOn = false;
    return 0;
  }

  if (!lastSwitchOn) {
    switchRampStartMs = nowMs;
    lastSwitchOn = true;
  }

  uint16_t rampMs = paramValues[PARAM_RAMP_MS];
  uint16_t target = paramValues[PARAM_SWITCH_MAX];
  unsigned long elapsedMs = nowMs - switchRampStartMs;

  if (rampMs == 0 || elapsedMs >= rampMs) {
    return target;
  }

  return ((uint32_t)target * elapsedMs) / rampMs;
}

bool changedEnough(uint16_t currentValue, uint16_t lastValue, uint16_t threshold) {
  return abs((int)currentValue - (int)lastValue) > (int)threshold;
}

bool flagIsSet(uint8_t flags, uint8_t bit) {
  return (flags & (1 << bit)) != 0;
}

/* ---------------------------------------------------------------- parameters */

uint16_t clampParam(uint8_t paramId, long value) {
  const ParamSpec& spec = paramSpecs[paramId];

  if (value < (long)spec.minValue) {
    return spec.minValue;
  }

  if (value > (long)spec.maxValue) {
    return spec.maxValue;
  }

  return (uint16_t)value;
}

void loadDefaultParams() {
  for (uint8_t i = 0; i < PARAM_COUNT; i++) {
    paramValues[i] = paramSpecs[i].defaultValue;
  }

  paramsFromNvs = false;
}

void applyParamsToHardware() {
  uint32_t freq = paramValues[PARAM_PWM_FREQ];
  uint8_t resolution = (uint8_t)paramValues[PARAM_PWM_RES];

  if (resolution < 8) {
    resolution = 8;
  }

  if (resolution > 14) {
    resolution = 14;
  }

  dutyFullScale = (1UL << resolution) - 1;

  for (int i = 0; i < 6; i++) {
    if (!ledcChangeFrequency(pwmPins[i], freq, resolution)) {
      pwmAttachOk[i] = ledcAttach(pwmPins[i], freq, resolution);
    }
  }
}

void saveParamsToNvs() {
  ParamBlob blob = {};
  blob.magic = paramBlobMagic;

  for (uint8_t i = 0; i < PARAM_COUNT; i++) {
    blob.values[i] = paramValues[i];
  }

  preferences.begin("pot2pwm", false);
  size_t written = preferences.putBytes("params", &blob, sizeof(blob));
  preferences.end();
  Serial.print("NVS: saved ");
  Serial.print(written);
  Serial.println(" bytes of parameters");
}

bool loadParamsFromNvs(bool verbose) {
  preferences.begin("pot2pwm", true);
  size_t length = preferences.getBytesLength("params");
  ParamBlob blob = {};
  bool ok = false;

  if (length == sizeof(blob)) {
    if (preferences.getBytes("params", &blob, sizeof(blob)) == sizeof(blob) && blob.magic == paramBlobMagic) {
      for (uint8_t i = 0; i < PARAM_COUNT; i++) {
        paramValues[i] = clampParam(i, blob.values[i]);
      }

      paramsFromNvs = true;
      ok = true;
    }
  }

  preferences.end();

  if (verbose) {
    Serial.print("NVS: load ");
    Serial.println(ok ? "ok" : "no saved parameters, using compiled defaults");
  }

  if (!ok) {
    loadDefaultParams();
  }

  return ok;
}

/* ------------------------------------------------------------------ overrides */

void applySet(uint8_t variable, uint16_t value) {
  int potIndex = -1;

  if (variable == variablePot1) {
    potIndex = 0;
  } else if (variable == variablePot2) {
    potIndex = 1;
  } else if (variable == variablePot3) {
    potIndex = 2;
  }

  if (potIndex >= 0) {
    potOverrides[potIndex].enabled = true;
    potOverrides[potIndex].value = clampAdc(value);
    return;
  }

  if (variable == variableSwitch) {
    switchOverride.enabled = true;
    switchOverride.value = value != 0;
  }
}

void applyClear(uint8_t variable) {
  if (variable == variableAll) {
    for (int i = 0; i < 3; i++) {
      potOverrides[i].enabled = false;
    }

    switchOverride.enabled = false;
    return;
  }

  if (variable == variablePot1) {
    potOverrides[0].enabled = false;
  } else if (variable == variablePot2) {
    potOverrides[1].enabled = false;
  } else if (variable == variablePot3) {
    potOverrides[2].enabled = false;
  } else if (variable == variableSwitch) {
    switchOverride.enabled = false;
  }
}

void applyForcePwm(uint8_t channel, uint16_t value) {
  if (channel >= 1 && channel <= 6) {
    pwmForce[channel - 1].enabled = true;
    pwmForce[channel - 1].value = clampAdc(value);
  }
}

void applyForceDo(uint8_t channel, uint16_t value) {
  if (channel >= 1 && channel <= 6) {
    doForce[channel - 1].enabled = true;
    doForce[channel - 1].value = value != 0;
  }
}

void applyReleasePwm(uint8_t channel) {
  if (channel == variableAll) {
    for (int i = 0; i < 6; i++) {
      pwmForce[i].enabled = false;
    }

    return;
  }

  if (channel >= 1 && channel <= 6) {
    pwmForce[channel - 1].enabled = false;
  }
}

void applyReleaseDo(uint8_t channel) {
  if (channel == variableAll) {
    for (int i = 0; i < 6; i++) {
      doForce[i].enabled = false;
    }

    return;
  }

  if (channel >= 1 && channel <= 6) {
    doForce[channel - 1].enabled = false;
  }
}

void applyReleaseAll() {
  applyClear(variableAll);
  applyReleasePwm(variableAll);
  applyReleaseDo(variableAll);
}

bool applySetParam(uint8_t paramId, uint16_t value) {
  if (paramId >= PARAM_COUNT) {
    return false;
  }

  uint16_t clamped = clampParam(paramId, value);
  paramValues[paramId] = clamped;

  if (paramId == PARAM_PWM_FREQ || paramId == PARAM_PWM_RES) {
    applyParamsToHardware();
  }

  return true;
}

int paramIdForName(const String& name) {
  for (uint8_t i = 0; i < PARAM_COUNT; i++) {
    if (name == paramSpecs[i].name) {
      return i;
    }
  }

  return -1;
}

/* ------------------------------------------------------------------- printing */

void printTelemetryHeader(Print& out) {
  out.println();
  out.println(F("SEQ   MS       BOARD  SW   CH    GPIO  DUTY  DUTY%  AVG_V  SRC    LEDC"));
  out.println(F("----  -------  -----  ---  ----  ----  ----  -----  -----  -----  ----"));
}

void printPwmTelemetryRow(Print& out,
                          uint8_t boardNumber,
                          bool switchRaw,
                          uint8_t channelIndex,
                          uint8_t gpio,
                          uint16_t used,
                          bool overridden,
                          uint16_t duty,
                          bool attached) {
  out.printf("%4u  %7lu  B%-4u  %-3s  PWM%-1u  %4u  %4u  %5.1f  %5.2f  %-6s %s\n",
             sequence,
             millis(),
             boardNumber,
             switchRaw ? "ON" : "OFF",
             channelIndex + 1,
             gpio,
             duty,
             dutyPercent(duty),
             outputVoltage(duty),
             channelSourceName(channelIndex),
             attached ? "OK" : "FAIL");
}

void printSwitchTelemetryRow(Print& out,
                             uint16_t switchDuty,
                             const uint16_t potValues[3],
                             const bool switchRawValues[3],
                             bool switchRaw,
                             bool switchOn,
                             uint8_t doStateMask,
                             bool pwm23Changing) {
  (void)switchRaw;
  out.printf("%4u  %7lu  ALL    SW %c%c%c  %-6s DO %02X  %4u  %5.2fV  P23_%-6s POT %4u %4u %4u\n",
             sequence,
             millis(),
             switchRawValues[0] ? '1' : '-',
             switchRawValues[1] ? '1' : '-',
             switchRawValues[2] ? '1' : '-',
             switchOverride.enabled ? "MAN" : (switchOn ? "ON" : "OFF"),
             doStateMask,
             switchDuty,
             outputVoltage(switchDuty),
             pwm23Changing ? "MOVING" : "STABLE",
             potValues[0],
             potValues[1],
             potValues[2]);
}

/* --------------------------------------------------------------- ESP-NOW send */

uint8_t flagsForState(bool switchRaw, bool switchOn, bool pwm23Changing) {
  uint8_t flags = 0;

  if (potOverrides[0].enabled) {
    flags |= 1 << 0;
  }

  if (potOverrides[1].enabled) {
    flags |= 1 << 1;
  }

  if (potOverrides[2].enabled) {
    flags |= 1 << 2;
  }

  if (switchOverride.enabled) {
    flags |= 1 << 3;
  }

  if (switchRaw) {
    flags |= 1 << 4;
  }

  if (switchOn) {
    flags |= 1 << 5;
  }

  if (pwm23Changing) {
    flags |= 1 << 6;
  }

  for (int i = 0; i < 6; i++) {
    if (pwmForce[i].enabled || doForce[i].enabled) {
      flags |= 1 << 7;
      break;
    }
  }

  return flags;
}

uint8_t maskForPwmForces() {
  uint8_t mask = 0;

  for (int i = 0; i < 6; i++) {
    if (pwmForce[i].enabled) {
      mask |= 1 << i;
    }
  }

  return mask;
}

uint8_t maskForDoForces() {
  uint8_t mask = 0;

  for (int i = 0; i < 6; i++) {
    if (doForce[i].enabled) {
      mask |= 1 << i;
    }
  }

  return mask;
}

void fillParamFields(EspNowPacket& packet) {
  packet.paramCount = PARAM_COUNT;

  for (uint8_t i = 0; i < PARAM_COUNT; i++) {
    packet.params[i] = paramValues[i];
    packet.paramMin[i] = paramSpecs[i].minValue;
    packet.paramMax[i] = paramSpecs[i].maxValue;
    packet.paramStep[i] = paramSpecs[i].step;
  }
}

void sendTelemetryPacket(const uint16_t potValues[3],
                         const uint16_t channelDuty[6],
                         uint16_t switchDuty,
                         const bool switchRawValues[3],
                         bool switchRaw,
                         bool switchOn,
                         uint8_t doStateMask,
                         bool pwm23Changing) {
  EspNowPacket packet = {};
  packet.magic = packetMagic;
  packet.version = packetVersion;
  packet.kind = packetKindTelemetry;
  packet.seq = sequence;

  for (int i = 0; i < 3; i++) {
    packet.potValue[i] = potValues[i];
  }

  for (int i = 0; i < 6; i++) {
    packet.pwmDuty[i] = channelDuty[i];
  }

  packet.switchDuty = switchDuty;
  packet.flags = flagsForState(switchRaw, switchOn, pwm23Changing);
  packet.pwmForceMask = maskForPwmForces();
  packet.doForceMask = maskForDoForces();
  packet.doStateMask = doStateMask;
  packet.switchRawMask = (switchRawValues[0] ? 1 : 0) | (switchRawValues[1] ? 2 : 0) | (switchRawValues[2] ? 4 : 0);
  fillParamFields(packet);
  esp_now_send(broadcastAddress, (const uint8_t*)&packet, sizeof(packet));
}

/* ------------------------------------------------------------ serial commands */

int potIndexForName(const String& name) {
  if (name == "pot1") {
    return 0;
  }

  if (name == "pot2") {
    return 1;
  }

  if (name == "pot3") {
    return 2;
  }

  return -1;
}

bool parseBoolValue(const String& valueText, bool& value) {
  if (valueText == "1" || valueText == "on" || valueText == "true") {
    value = true;
    return true;
  }

  if (valueText == "0" || valueText == "off" || valueText == "false") {
    value = false;
    return true;
  }

  return false;
}

void reply(Print& out, const __FlashStringHelper* status, const String& detail) {
  out.print(status);
  out.print(' ');
  out.println(detail);
}

void printParamTable(Print& out) {
  out.println(F("PARAM   VALUE   MIN     MAX     STEP    DEFAULT"));
  out.println(F("------  ------  ------  ------  ------  -------"));

  for (uint8_t i = 0; i < PARAM_COUNT; i++) {
    char line[80];
    snprintf(line, sizeof(line), "%-6s  %6u  %6u  %6u  %6u  %7u",
             paramSpecs[i].name,
             paramValues[i],
             paramSpecs[i].minValue,
             paramSpecs[i].maxValue,
             paramSpecs[i].step,
             paramSpecs[i].defaultValue);
    out.println(line);
  }
}

void printStatusSummary(Print& out) {
  out.print(F("params: "));
  out.println(paramsFromNvs ? F("loaded from NVS") : F("compiled defaults"));
  out.print(F("pots: "));
  for (int i = 0; i < 3; i++) {
    out.print(potOverrides[i].enabled ? String(potOverrides[i].value) : String("auto"));
    out.print(i < 2 ? " " : "\n");
  }
  out.print(F("switch: "));
  out.println(switchOverride.enabled ? (switchOverride.value ? "MAN ON" : "MAN OFF") : "auto");
  out.print(F("pwm forces: "));
  out.println(maskForPwmForces(), HEX);
  out.print(F("do forces: "));
  out.println(maskForDoForces(), HEX);
}

void handleSetCommand(const String& variable, const String& valueText, Print& out) {
  int potIndex = potIndexForName(variable);

  if (potIndex >= 0) {
    applySet((uint8_t)(variablePot1 + potIndex), clampAdc(valueText.toInt()));
    reply(out, F("OK"), variable + "=" + String(potOverrides[potIndex].value));
    return;
  }

  if (variable == "switch") {
    bool parsedValue = false;

    if (!parseBoolValue(valueText, parsedValue)) {
      reply(out, F("ERR"), F("switch expects 0|1|on|off"));
      return;
    }

    applySet(variableSwitch, parsedValue ? 1 : 0);
    reply(out, F("OK"), String("switch=") + (switchOverride.value ? "ON" : "OFF"));
    return;
  }

  reply(out, F("ERR"), F("unknown variable"));
}

void handleClearCommand(const String& variable, Print& out) {
  if (variable == "all") {
    applyReleaseAll();
    reply(out, F("OK"), F("all overrides and forces cleared"));
    return;
  }

  int potIndex = potIndexForName(variable);

  if (potIndex >= 0) {
    applyClear((uint8_t)(variablePot1 + potIndex));
    reply(out, F("OK"), variable + String(" auto"));
    return;
  }

  if (variable == "switch") {
    applyClear(variableSwitch);
    reply(out, F("OK"), F("switch auto"));
    return;
  }

  reply(out, F("ERR"), F("unknown variable"));
}

void handleForcePwmCommand(const String& target, const String& valueText, Print& out) {
  int channel = target.toInt();

  if (channel < 1 || channel > 6) {
    reply(out, F("ERR"), F("usage: PWM 1..6 value|off"));
    return;
  }

  if (valueText == "off" || valueText == "auto") {
    applyReleasePwm((uint8_t)channel);
    reply(out, F("OK"), String("PWM") + channel + F(" released"));
    return;
  }

  applyForcePwm((uint8_t)channel, clampAdc(valueText.toInt()));
  reply(out, F("OK"), String("PWM") + channel + "=" + String(pwmForce[channel - 1].value));
}

void handleForceDoCommand(const String& target, const String& valueText, Print& out) {
  int channel = target.toInt();

  if (channel < 1 || channel > 6) {
    reply(out, F("ERR"), F("usage: DO 1..6 0|1|off"));
    return;
  }

  if (valueText == "off" || valueText == "auto") {
    applyReleaseDo((uint8_t)channel);
    reply(out, F("OK"), String("DO") + channel + F(" released"));
    return;
  }

  bool parsedValue = false;

  if (!parseBoolValue(valueText, parsedValue)) {
    reply(out, F("ERR"), F("DO expects 0|1|on|off"));
    return;
  }

  applyForceDo((uint8_t)channel, parsedValue ? 1 : 0);
  reply(out, F("OK"), String("DO") + channel + (doForce[channel - 1].value ? F("=ON") : F("=OFF")));
}

void handleParamCommand(const String& rest, Print& out) {
  if (rest.length() == 0) {
    printParamTable(out);
    return;
  }

  int secondSpace = rest.indexOf(' ');
  String name = secondSpace < 0 ? rest : rest.substring(0, secondSpace);
  String valueText = secondSpace < 0 ? "" : rest.substring(secondSpace + 1);
  name.trim();
  valueText.trim();
  int paramId = paramIdForName(name);

  if (paramId < 0) {
    reply(out, F("ERR"), F("unknown parameter"));
    return;
  }

  if (valueText.length() == 0) {
    reply(out, F("OK"),
          name + "=" + String(paramValues[paramId])
               + " min " + String(paramSpecs[paramId].minValue)
               + " max " + String(paramSpecs[paramId].maxValue)
               + " step " + String(paramSpecs[paramId].step)
               + " default " + String(paramSpecs[paramId].defaultValue));
    return;
  }

  applySetParam((uint8_t)paramId, (uint16_t)valueText.toInt());
  reply(out, F("OK"), name + "=" + String(paramValues[paramId]));
}

void handleCommand(String line, Print& out) {
  line.trim();

  if (line.length() == 0) {
    return;
  }

  line.toLowerCase();

  int firstSpace = line.indexOf(' ');
  String command = firstSpace < 0 ? line : line.substring(0, firstSpace);
  String rest = firstSpace < 0 ? "" : line.substring(firstSpace + 1);
  rest.trim();

  if (command == "set") {
    int secondSpace = rest.indexOf(' ');

    if (secondSpace < 0) {
      reply(out, F("ERR"), F("usage: SET pot1|pot2|pot3|switch value"));
      return;
    }

    String variable = rest.substring(0, secondSpace);
    String valueText = rest.substring(secondSpace + 1);
    variable.trim();
    valueText.trim();
    handleSetCommand(variable, valueText, out);
    return;
  }

  if (command == "clr" || command == "clear") {
    if (rest.length() == 0) {
      reply(out, F("ERR"), F("usage: CLR pot1|pot2|pot3|switch|all"));
      return;
    }

    handleClearCommand(rest, out);
    return;
  }

  if (command == "pwm") {
    int secondSpace = rest.indexOf(' ');

    if (secondSpace < 0) {
      reply(out, F("ERR"), F("usage: PWM 1..6 value|off"));
      return;
    }

    String target = rest.substring(0, secondSpace);
    String valueText = rest.substring(secondSpace + 1);
    target.trim();
    valueText.trim();
    handleForcePwmCommand(target, valueText, out);
    return;
  }

  if (command == "do") {
    int secondSpace = rest.indexOf(' ');

    if (secondSpace < 0) {
      reply(out, F("ERR"), F("usage: DO 1..6 0|1|off"));
      return;
    }

    String target = rest.substring(0, secondSpace);
    String valueText = rest.substring(secondSpace + 1);
    target.trim();
    valueText.trim();
    handleForceDoCommand(target, valueText, out);
    return;
  }

  if (command == "param") {
    handleParamCommand(rest, out);
    return;
  }

  if (command == "save") {
    saveParamsToNvs();
    reply(out, F("OK"), F("parameters saved"));
    return;
  }

  if (command == "load") {
    loadParamsFromNvs(true);
    applyParamsToHardware();
    reply(out, F("OK"), F("parameters loaded"));
    return;
  }

  if (command == "defaults") {
    loadDefaultParams();
    applyParamsToHardware();
    reply(out, F("OK"), F("parameters reset to compiled defaults"));
    return;
  }

  if (command == "release") {
    applyReleaseAll();
    reply(out, F("OK"), F("everything released"));
    return;
  }

  if (command == "status") {
    printParamTable(out);
    printStatusSummary(out);
    return;
  }

  reply(out, F("ERR"), F("unknown command"));
}

void readCommands(Stream& stream, String& lineBuffer, Print& out) {
  while (stream.available() > 0) {
    char c = stream.read();

    if (c == '\n' || c == '\r') {
      if (lineBuffer.length() > 0) {
        handleCommand(lineBuffer, out);
        lineBuffer = "";
      }
    } else if (lineBuffer.length() < commandMaxLength) {
      lineBuffer += c;
    } else {
      lineBuffer = "";
      reply(out, F("ERR"), F("command too long"));
    }
  }
}

/* --------------------------------------------------------- ESP-NOW commands */

void onEspNowReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  (void)info;

  if (len != sizeof(EspNowPacket)) {
    return;
  }

  EspNowPacket packet;
  memcpy(&packet, data, sizeof(packet));

  if (packet.magic != packetMagic || packet.version != packetVersion || packet.kind != packetKindCommand) {
    return;
  }

  portENTER_CRITICAL(&pendingCommandMux);
  pendingCommandPacket = packet;
  havePendingCommandPacket = true;
  portEXIT_CRITICAL(&pendingCommandMux);
}

void processEspNowCommand() {
  if (!havePendingCommandPacket) {
    return;
  }

  EspNowPacket packet;

  portENTER_CRITICAL(&pendingCommandMux);
  packet = pendingCommandPacket;
  havePendingCommandPacket = false;
  portEXIT_CRITICAL(&pendingCommandMux);

  switch (packet.command) {
    case commandSet:
      applySet(packet.variable, packet.commandValue);
      Serial.print(F("ESP-NOW SET variable="));
      Serial.print(packet.variable);
      Serial.print(F(" value="));
      Serial.println(packet.commandValue);
      break;

    case commandClear:
      applyClear(packet.variable);
      Serial.print(F("ESP-NOW CLR variable="));
      Serial.println(packet.variable);
      break;

    case commandForcePwm:
      applyForcePwm(packet.variable, packet.commandValue);
      Serial.print(F("ESP-NOW FORCE PWM"));
      Serial.print(packet.variable);
      Serial.print('=');
      Serial.println(packet.commandValue);
      break;

    case commandForceDo:
      applyForceDo(packet.variable, packet.commandValue);
      Serial.print(F("ESP-NOW FORCE DO"));
      Serial.print(packet.variable);
      Serial.print('=');
      Serial.println(packet.commandValue);
      break;

    case commandReleasePwm:
      applyReleasePwm(packet.variable);
      Serial.print(F("ESP-NOW RELEASE PWM "));
      Serial.println(packet.variable);
      break;

    case commandReleaseDo:
      applyReleaseDo(packet.variable);
      Serial.print(F("ESP-NOW RELEASE DO "));
      Serial.println(packet.variable);
      break;

    case commandSetParam:
      if (applySetParam(packet.variable, packet.commandValue)) {
        Serial.print(F("ESP-NOW PARAM "));
        Serial.print(paramSpecs[packet.variable].name);
        Serial.print('=');
        Serial.println(paramValues[packet.variable]);
      }
      break;

    case commandSaveParams:
      saveParamsToNvs();
      break;

    case commandLoadParams:
      loadParamsFromNvs(false);
      applyParamsToHardware();
      Serial.println(F("ESP-NOW params loaded from NVS"));
      break;

    case commandResetParams:
      loadDefaultParams();
      applyParamsToHardware();
      Serial.println(F("ESP-NOW params reset to defaults"));
      break;

    case commandReleaseAll:
      applyReleaseAll();
      Serial.println(F("ESP-NOW release all"));
      break;

    default:
      break;
  }
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(espNowChannel);

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("ESP-NOW init failed"));
    return;
  }

  esp_now_register_recv_cb(onEspNowReceive);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, sizeof(broadcastAddress));
  peerInfo.channel = espNowChannel;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(broadcastAddress)) {
    esp_err_t result = esp_now_add_peer(&peerInfo);

    if (result != ESP_OK) {
      Serial.print(F("ESP-NOW broadcast peer failed: "));
      Serial.println(result);
      return;
    }
  }

  Serial.print(F("ESP-NOW ready on channel "));
  Serial.print(espNowChannel);
  Serial.print(F(" MAC "));
  Serial.println(WiFi.macAddress());
}

/* ---------------------------------------------------------------------- main */

void printTelemetry(Print& out,
                    const uint16_t potValues[3],
                    const uint16_t channelDuty[6],
                    uint16_t switchDuty,
                    const bool switchRawValues[3],
                    bool switchRaw,
                    bool switchOn,
                    uint8_t doStateMask,
                    bool pwm23Changing) {
  if (sequence % telemetryHeaderEvery == 0) {
    printTelemetryHeader(out);
  }

  for (int board = 0; board < 3; board++) {
    for (int output = 0; output < 2; output++) {
      uint8_t channelIndex = boardPwmIndexes[board][output];
      printPwmTelemetryRow(out,
                           board + 1,
                           switchRawValues[board],
                           channelIndex,
                           pwmPins[channelIndex],
                           potValues[board],
                           potOverrides[board].enabled || pwmForce[channelIndex].enabled,
                           channelDuty[channelIndex],
                           pwmAttachOk[channelIndex]);
    }
  }

  printSwitchTelemetryRow(out, switchDuty, potValues, switchRawValues, switchRaw, switchOn, doStateMask, pwm23Changing);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println(F("BOOT: Standalone ESP32 38-pin pot-to-PWM (protocol v2)"));
  loadDefaultParams();
  loadParamsFromNvs(true);
  setupEspNow();

  Serial.println(F("BOOT: attaching PWM pins"));
  for (int i = 0; i < 6; i++) {
    pinMode(pwmPins[i], OUTPUT);
    pwmAttachOk[i] = ledcAttach(pwmPins[i], paramValues[PARAM_PWM_FREQ], (uint8_t)paramValues[PARAM_PWM_RES]);
    Serial.print(F("  PWM pin GPIO"));
    Serial.print(pwmPins[i]);
    Serial.print(F(" -> "));
    Serial.println(pwmAttachOk[i] ? F("OK") : F("FAIL"));
    pinMode(digitalOutPins[i], OUTPUT);
    digitalWrite(digitalOutPins[i], LOW);
    ledcWrite(pwmPins[i], 0);
  }

  dutyFullScale = (1UL << (uint8_t)paramValues[PARAM_PWM_RES]) - 1;

  for (int i = 0; i < 3; i++) {
    pinMode(switchInputPins[i], INPUT_PULLDOWN);
  }

  startupMs = millis();
}

void loop() {
  unsigned long nowMs = millis();
  readCommands(Serial, usbCommandLine, Serial);
  processEspNowCommand();

  uint16_t potInputs[3];

  // Pot direction. potflip inverts all three inputs; inv1..inv3 invert a single slot.
  // The two XOR, so a per-slot flag stays an individual correction while potflip is on:
  // set potflip to fix a whole harness at once, then clear the odd slot with its invN.
  bool globalPotFlip = paramValues[PARAM_POT_FLIP] != 0;

  for (int i = 0; i < 3; i++) {
    uint16_t rawPot = (uint16_t)analogRead(potPins[i]);
    bool flip = (globalPotFlip != (paramValues[PARAM_INVERT_POT1 + i] != 0));
    potInputs[i] = flip ? (uint16_t)(adcMax - rawPot) : rawPot;
  }
  bool switchRawValues[3] = {false, false, false};
  bool switchRaw = false;

  for (int i = 0; i < 3; i++) {
    switchRawValues[i] = digitalRead(switchInputPins[i]) == HIGH;

    if (switchRawValues[i]) {
      switchRaw = true;
    }
  }

  uint16_t potValues[3];

  for (int i = 0; i < 3; i++) {
    potValues[i] = potOverrides[i].enabled ? potOverrides[i].value : potInputs[i];
  }

  bool switchOn = switchOverride.enabled ? switchOverride.value : switchRaw;
  uint16_t moveThreshold = paramValues[PARAM_MOVE_THRESH];

  if (!haveLastPots) {
    lastPot2 = potValues[1];
    lastPot3 = potValues[2];
    lastMovementSampleMs = nowMs;
    haveLastPots = true;
  } else if (nowMs - lastMovementSampleMs >= movementSampleMs) {
    if (changedEnough(potValues[1], lastPot2, moveThreshold) || changedEnough(potValues[2], lastPot3, moveThreshold)) {
      lastPwm23ChangeMs = nowMs;
    }

    lastPot2 = potValues[1];
    lastPot3 = potValues[2];
    lastMovementSampleMs = nowMs;
  }

  bool pwm23Changing = nowMs - lastPwm23ChangeMs <= paramValues[PARAM_MOVE_HOLD_MS];
  uint16_t boardDuty[3] = {
    startupRampedDuty(cappedDuty(sensitiveDuty(potValues[0]), paramValues[PARAM_MAX_B1]), nowMs),
    startupRampedDuty(cappedDuty(sensitiveDuty(potValues[1]), paramValues[PARAM_MAX_B2]), nowMs),
    startupRampedDuty(cappedDuty(sensitiveDuty(potValues[2]), paramValues[PARAM_MAX_B3]), nowMs)
  };
  uint16_t switchDuty = switchRampedDuty(switchOn, nowMs);
  uint16_t channelDuty[6];

  for (int i = 0; i < 6; i++) {
    channelDuty[i] = pwmForce[i].enabled ? pwmForce[i].value : boardDuty[i / 2];
    ledcWrite(pwmPins[i], scaleDuty(channelDuty[i]));
  }

  uint8_t doStateMask = 0;

  for (int i = 0; i < 3; i++) {
    bool outputOn = switchOverride.enabled ? switchOn : switchRawValues[i];

    for (int output = 0; output < 2; output++) {
      int doIndex = i * 2 + output;
      bool level = doForce[doIndex].enabled ? doForce[doIndex].value : outputOn;
      digitalWrite(digitalOutPins[doIndex], level ? HIGH : LOW);

      if (level) {
        doStateMask |= 1 << doIndex;
      }
    }
  }

  if (nowMs - lastReportMs >= paramValues[PARAM_REPORT_MS]) {
    lastReportMs = nowMs;

    printTelemetry(Serial, potValues, channelDuty, switchDuty, switchRawValues, switchRaw, switchOn, doStateMask, pwm23Changing);
    sendTelemetryPacket(potValues, channelDuty, switchDuty, switchRawValues, switchRaw, switchOn, doStateMask, pwm23Changing);
    sequence++;
  }
}
