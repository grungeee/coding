/*
  Standalone ESP32 38-pin potentiometer to PWM converter.

  Local analog inputs:
    pot1 -> GPIO34
    pot2 -> GPIO35
    pot3 -> GPIO36
    Pots use ADC1 pins so they keep working while WiFi/ESP-NOW is active.

  I2C:
    Not assigned in this 38-pin core-functionality pinout.

  Switch inputs:
    switch1 -> GPIO32
    switch2 -> GPIO33
    switch3 -> GPIO4
    Switch input is ON when connected to 3.3 V.
    INPUT_PULLDOWN is used, so LOW=released/off and HIGH=pressed/on.

  Digital outputs:
    switch1 high -> GPIO13, GPIO14 high
    switch2 high -> GPIO16, GPIO17 high
    switch3 high -> GPIO18, GPIO19 high

  PWM outputs:
    board1/switch1 -> PWM1 GPIO21, PWM2 GPIO22
    board2/switch2 -> PWM3 GPIO23, PWM4 GPIO25
    board3/switch3 -> PWM5 GPIO26, PWM6 GPIO27

  Serial output at 115200 baud prints ADC values and converted output voltages.

  Optional Cardputer controller UI link uses ESP-NOW broadcast.

  Runtime override commands, newline terminated:
    SET pot1 0..4095
    SET pot2 0..4095
    SET pot3 0..4095
    SET switch 0|1
    CLR pot1|pot2|pot3|switch|all
*/

#include <WiFi.h>
#include <esp_now.h>

const int potPins[3] = {34, 35, 36};
const int digitalOutPins[6] = {13, 14, 16, 17, 18, 19};
const int switchDigitalOutPins[3][2] = {
  {13, 14},
  {16, 17},
  {18, 19}
};
const int pwmPins[6] = {21, 22, 23, 25, 26, 27};
const uint8_t boardPwmIndexes[3][2] = {
  {0, 1},
  {2, 3},
  {4, 5}
};
const int switchInputPins[3] = {32, 33, 4};

const int pwmFreq = 15000;
const int pwmResolution = 12;
const int adcMax = 4095;
const int pwm1Max = (adcMax * 50) / 100;
const int pwm23Max = (adcMax * 50) / 100;
const float fullScaleVoltage = 3.3f;
const uint16_t potLowInput = 100;
const uint16_t potLowDuty = 559;
const uint16_t potHighInput = adcMax;
const uint16_t potHighDuty = adcMax / 2;
const unsigned long reportIntervalMs = 100;
const unsigned long startupRampMs = 5000;
const unsigned long movementSampleMs = 50;
const unsigned long movementHoldMs = 500;
const int movementThreshold = 16;
const uint8_t telemetryHeaderEvery = 20;
const size_t commandMaxLength = 64;
const uint8_t espNowChannel = 1;
const uint32_t packetMagic = 0x50325057;
const uint8_t packetVersion = 1;
const uint8_t packetKindTelemetry = 1;
const uint8_t packetKindCommand = 2;
const uint8_t commandSet = 1;
const uint8_t commandClear = 2;
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
  uint16_t pwmDuty[3];
  uint16_t switchDuty;
  uint8_t flags;
} __attribute__((packed));

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
String usbCommandLine;
EspNowPacket pendingCommandPacket;
volatile bool havePendingCommandPacket = false;
portMUX_TYPE pendingCommandMux = portMUX_INITIALIZER_UNLOCKED;
bool pwmAttachOk[6] = {false, false, false, false, false, false};

struct AdcOverride {
  bool enabled = false;
  uint16_t value = 0;
};

struct BoolOverride {
  bool enabled = false;
  bool value = false;
};

AdcOverride potOverrides[3];
BoolOverride switchOverride;

float outputVoltage(uint16_t value) {
  return (value * fullScaleVoltage) / adcMax;
}

uint16_t cappedDuty(uint16_t value, uint16_t maxValue) {
  return value > maxValue ? maxValue : value;
}

uint16_t sensitiveDuty(uint16_t value) {
  if (value <= potLowInput) {
    return ((uint32_t)value * potLowDuty + (potLowInput / 2)) / potLowInput;
  }

  uint32_t inputSpan = potHighInput - potLowInput;
  uint32_t dutySpan = potHighDuty - potLowDuty;
  uint32_t scaled = potLowDuty + (((uint32_t)(value - potLowInput) * dutySpan + (inputSpan / 2)) / inputSpan);
  return scaled > adcMax ? adcMax : (uint16_t)scaled;
}

uint16_t startupRampedDuty(uint16_t value, unsigned long nowMs) {
  unsigned long elapsedMs = nowMs - startupMs;

  if (elapsedMs >= startupRampMs) {
    return value;
  }

  uint32_t rampMax = ((uint32_t)adcMax * elapsedMs) / startupRampMs;
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

  unsigned long elapsedMs = nowMs - switchRampStartMs;

  if (elapsedMs >= startupRampMs) {
    return adcMax;
  }

  return ((uint32_t)adcMax * elapsedMs) / startupRampMs;
}

bool changedEnough(uint16_t currentValue, uint16_t lastValue) {
  return abs((int)currentValue - (int)lastValue) > movementThreshold;
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

const char* sourceName(bool overridden) {
  return overridden ? "MAN" : "AUTO";
}

float dutyPercent(uint16_t duty) {
  return (duty * 100.0f) / adcMax;
}

void printTelemetryHeader(Print& out) {
  out.println();
  out.println(F("SEQ   MS       BOARD  SW   PWM   GPIO  POT   SRC   DUTY  DUTY%  AVG_V  LEDC"));
  out.println(F("----  -------  -----  ---  ----  ----  ----  ----  ----  -----  -----  ----"));
}

void printPwmTelemetryRow(Print& out,
                          uint8_t boardNumber,
                          bool switchRaw,
                          uint8_t pwmIndex,
                          uint8_t gpio,
                          uint16_t used,
                          bool overridden,
                          uint16_t duty,
                          bool attached) {
  out.printf("%4u  %7lu  B%-4u  %-3s  PWM%-1u  %4u  %4u  %-4s  %4u  %5.1f  %5.2f  %s\n",
             sequence,
             millis(),
             boardNumber,
             switchRaw ? "ON" : "OFF",
             pwmIndex + 1,
             gpio,
             used,
             sourceName(overridden),
             duty,
             dutyPercent(duty),
             outputVoltage(duty),
             attached ? "OK" : "FAIL");
}

void printSwitchTelemetryRow(Print& out,
                             uint16_t switchDuty,
                             bool switchRaw,
                             bool switchOn,
                             bool overridden,
                             bool pwm23Changing) {
  out.printf("%4u  %7lu  ALL    %-3s  SW    ----  ----  %-4s  %4u  %5.1f  %5.2f  P23_%s\n",
             sequence,
             millis(),
             switchRaw ? "ON" : "OFF",
             switchOn ? "ON" : "OFF",
             sourceName(overridden),
             switchDuty,
             dutyPercent(switchDuty),
             outputVoltage(switchDuty),
             pwm23Changing ? "MOVING" : "STABLE");
}

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

  return flags;
}

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

int potIndexForVariable(uint8_t variable) {
  if (variable == variablePot1) {
    return 0;
  }

  if (variable == variablePot2) {
    return 1;
  }

  if (variable == variablePot3) {
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

void applySet(uint8_t variable, uint16_t value) {
  int potIndex = potIndexForVariable(variable);

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

  int potIndex = potIndexForVariable(variable);

  if (potIndex >= 0) {
    potOverrides[potIndex].enabled = false;
    return;
  }

  if (variable == variableSwitch) {
    switchOverride.enabled = false;
  }
}

void reply(Print& out, const __FlashStringHelper* status, const String& detail) {
  out.print(status);
  out.print(' ');
  out.println(detail);
}

void handleSetCommand(const String& variable, const String& valueText, Print& out) {
  int potIndex = potIndexForName(variable);

  if (potIndex >= 0) {
    applySet(variablePot1 + potIndex, clampAdc(valueText.toInt()));
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
    applyClear(variableAll);
    reply(out, F("OK"), F("all overrides cleared"));
    return;
  }

  int potIndex = potIndexForName(variable);

  if (potIndex >= 0) {
    applyClear(variablePot1 + potIndex);
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

  if (command == "status") {
    reply(out, F("OK"), F("status follows on next telemetry frame"));
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

  if (packet.command == commandSet) {
    applySet(packet.variable, packet.commandValue);
    Serial.print("ESP-NOW SET variable=");
    Serial.print(packet.variable);
    Serial.print(" value=");
    Serial.println(packet.commandValue);
  } else if (packet.command == commandClear) {
    applyClear(packet.variable);
    Serial.print("ESP-NOW CLR variable=");
    Serial.println(packet.variable);
  }
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(espNowChannel);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
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
      Serial.print("ESP-NOW broadcast peer failed: ");
      Serial.println(result);
      return;
    }
  }

  Serial.print("ESP-NOW ready on channel ");
  Serial.print(espNowChannel);
  Serial.print(" MAC ");
  Serial.println(WiFi.macAddress());
}

void sendTelemetryPacket(uint16_t pot1,
                         uint16_t pot2,
                         uint16_t pot3,
                         uint16_t pwm1,
                         uint16_t pwm2,
                         uint16_t pwm3,
                         uint16_t switchDuty,
                         bool switchRaw,
                         bool switchOn,
                         bool pwm23Changing) {
  EspNowPacket packet = {};
  packet.magic = packetMagic;
  packet.version = packetVersion;
  packet.kind = packetKindTelemetry;
  packet.seq = sequence;
  packet.potValue[0] = pot1;
  packet.potValue[1] = pot2;
  packet.potValue[2] = pot3;
  packet.pwmDuty[0] = pwm1;
  packet.pwmDuty[1] = pwm2;
  packet.pwmDuty[2] = pwm3;
  packet.switchDuty = switchDuty;
  packet.flags = flagsForState(switchRaw, switchOn, pwm23Changing);
  esp_now_send(broadcastAddress, (const uint8_t*)&packet, sizeof(packet));
}

void printTelemetry(Print& out,
                    uint16_t pot1,
                    uint16_t pot2,
                    uint16_t pot3,
                    uint16_t pwm1,
                    uint16_t pwm2,
                    uint16_t pwm3,
                    uint16_t switchDuty,
                    const bool switchRawValues[3],
                    bool switchRaw,
                    bool switchOn,
                    bool pwm23Changing) {
  if (sequence % telemetryHeaderEvery == 0) {
    printTelemetryHeader(out);
  }

  const uint16_t usedValues[3] = {pot1, pot2, pot3};
  const uint16_t dutyValues[3] = {pwm1, pwm2, pwm3};

  for (int board = 0; board < 3; board++) {
    for (int output = 0; output < 2; output++) {
      uint8_t pwmIndex = boardPwmIndexes[board][output];
      printPwmTelemetryRow(out,
                           board + 1,
                           switchRawValues[board],
                           pwmIndex,
                           pwmPins[pwmIndex],
                           usedValues[board],
                           potOverrides[board].enabled,
                           dutyValues[board],
                           pwmAttachOk[pwmIndex]);
    }
  }

  printSwitchTelemetryRow(out, switchDuty, switchRaw, switchOn, switchOverride.enabled, pwm23Changing);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("BOOT: Standalone ESP32 38-pin pot-to-PWM");
  setupEspNow();

  Serial.println("BOOT: attaching PWM pins");
  for (int i = 0; i < 6; i++) {
    pinMode(pwmPins[i], OUTPUT);
    pwmAttachOk[i] = ledcAttach(pwmPins[i], pwmFreq, pwmResolution);
    Serial.print("  PWM pin GPIO");
    Serial.print(pwmPins[i]);
    Serial.print(" -> ");
    Serial.println(pwmAttachOk[i] ? "OK" : "FAIL");
    pinMode(digitalOutPins[i], OUTPUT);
    digitalWrite(digitalOutPins[i], LOW);
    ledcWrite(pwmPins[i], 0);
  }

  for (int i = 0; i < 3; i++) {
    pinMode(switchInputPins[i], INPUT_PULLDOWN);
  }

  startupMs = millis();
}

void loop() {
  unsigned long nowMs = millis();
  readCommands(Serial, usbCommandLine, Serial);
  processEspNowCommand();

  uint16_t pot1Input = analogRead(potPins[0]);
  uint16_t pot2Input = analogRead(potPins[1]);
  uint16_t pot3Input = analogRead(potPins[2]);
  bool switchRawValues[3] = {false, false, false};
  bool switchRaw = false;

  for (int i = 0; i < 3; i++) {
    switchRawValues[i] = digitalRead(switchInputPins[i]) == HIGH;

    if (switchRawValues[i]) {
      switchRaw = true;
    }
  }
  uint16_t pot1 = potOverrides[0].enabled ? potOverrides[0].value : pot1Input;
  uint16_t pot2 = potOverrides[1].enabled ? potOverrides[1].value : pot2Input;
  uint16_t pot3 = potOverrides[2].enabled ? potOverrides[2].value : pot3Input;
  bool switchOn = switchOverride.enabled ? switchOverride.value : switchRaw;

  if (!haveLastPots) {
    lastPot2 = pot2;
    lastPot3 = pot3;
    lastMovementSampleMs = nowMs;
    haveLastPots = true;
  } else if (nowMs - lastMovementSampleMs >= movementSampleMs) {
    if (changedEnough(pot2, lastPot2) || changedEnough(pot3, lastPot3)) {
      lastPwm23ChangeMs = nowMs;
    }

    lastPot2 = pot2;
    lastPot3 = pot3;
    lastMovementSampleMs = nowMs;
  }

  bool pwm23Changing = nowMs - lastPwm23ChangeMs <= movementHoldMs;
  uint16_t pwm1 = startupRampedDuty(cappedDuty(sensitiveDuty(pot1), pwm1Max), nowMs);
  uint16_t pwm2 = startupRampedDuty(cappedDuty(sensitiveDuty(pot2), pwm23Max), nowMs);
  uint16_t pwm3 = startupRampedDuty(cappedDuty(sensitiveDuty(pot3), pwm23Max), nowMs);
  uint16_t switchDuty = switchRampedDuty(switchOn, nowMs);

  uint16_t pwmValues[6] = {pwm1, pwm1, pwm2, pwm2, pwm3, pwm3};

  for (int i = 0; i < 6; i++) {
    ledcWrite(pwmPins[i], pwmValues[i]);
  }

  for (int i = 0; i < 3; i++) {
    bool outputOn = switchOverride.enabled ? switchOn : switchRawValues[i];
    digitalWrite(switchDigitalOutPins[i][0], outputOn ? HIGH : LOW);
    digitalWrite(switchDigitalOutPins[i][1], outputOn ? HIGH : LOW);
  }

  if (nowMs - lastReportMs >= reportIntervalMs) {
    lastReportMs = nowMs;

    printTelemetry(Serial, pot1, pot2, pot3, pwm1, pwm2, pwm3, switchDuty, switchRawValues, switchRaw, switchOn, pwm23Changing);
    sendTelemetryPacket(pot1, pot2, pot3, pwm1, pwm2, pwm3, switchDuty, switchRaw, switchOn, pwm23Changing);
    sequence++;
  }
}
