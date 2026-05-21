/*
  M5Stack Cardputer UI for the ESP32 pot-to-PWM controller.

  ESP-NOW broadcast link, no UART wiring or MAC configuration required.

  Cardputer arrow key characters in the M5Cardputer keyboard library:
    up=';' down='.' left=',' right='/'
*/

#include <M5Cardputer.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <esp_now.h>

const int adcMax = 4095;
const int potStep = 32;
const unsigned long firstRepeatDelayMs = 320;
const unsigned long repeatDelayMs = 70;
const unsigned long redrawIntervalMs = 120;
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
const uint8_t broadcastAddress[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct EspNowPacket {
  uint32_t magic;
  uint8_t version;
  uint8_t kind;
  uint16_t seq;
  uint8_t command;
  uint8_t variable;
  uint16_t commandValue;
  uint16_t potRaw[3];
  uint16_t potValue[3];
  uint16_t pwmDuty[3];
  uint16_t switchDuty;
  uint8_t flags;
} __attribute__((packed));

M5Canvas canvas(&M5Cardputer.Display);
String statusLine = "waiting for controller";

struct Telemetry {
  bool valid = false;
  uint32_t seq = 0;
  uint16_t potRaw[3] = {0, 0, 0};
  uint16_t potValue[3] = {0, 0, 0};
  uint16_t pwmDuty[3] = {0, 0, 0};
  float pwmVoltage[3] = {0.0f, 0.0f, 0.0f};
  bool potManual[3] = {false, false, false};
  bool switchRaw = false;
  bool switchValue = false;
  bool switchManual = false;
  float switchVoltage = 0.0f;
  bool pwm23Changing = false;
  unsigned long lastUpdateMs = 0;
};

Telemetry telemetry;

enum RowType {
  ROW_POT1,
  ROW_POT2,
  ROW_POT3,
  ROW_SWITCH,
  ROW_PWM1,
  ROW_PWM2,
  ROW_PWM3,
  ROW_MOVE,
  ROW_LINK,
  ROW_COUNT
};

int selectedRow = ROW_POT1;
int heldDirection = 0;
unsigned long nextRepeatMs = 0;
unsigned long lastRedrawMs = 0;
bool redrawNeeded = true;
uint16_t commandSeq = 0;
EspNowPacket pendingTelemetryPacket;
volatile bool havePendingTelemetryPacket = false;
portMUX_TYPE pendingTelemetryMux = portMUX_INITIALIZER_UNLOCKED;

uint16_t clampAdc(long value) {
  if (value < 0) {
    return 0;
  }

  if (value > adcMax) {
    return adcMax;
  }

  return (uint16_t)value;
}

float outputVoltage(uint16_t value) {
  return (value * 3.3f) / adcMax;
}

bool flagIsSet(uint8_t flags, uint8_t bit) {
  return (flags & (1 << bit)) != 0;
}

void onEspNowReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  (void)info;

  if (len != sizeof(EspNowPacket)) {
    return;
  }

  EspNowPacket packet;
  memcpy(&packet, data, sizeof(packet));

  if (packet.magic != packetMagic || packet.version != packetVersion || packet.kind != packetKindTelemetry) {
    return;
  }

  portENTER_CRITICAL(&pendingTelemetryMux);
  pendingTelemetryPacket = packet;
  havePendingTelemetryPacket = true;
  portEXIT_CRITICAL(&pendingTelemetryMux);
}

void processTelemetry() {
  if (!havePendingTelemetryPacket) {
    return;
  }

  EspNowPacket packet;

  portENTER_CRITICAL(&pendingTelemetryMux);
  packet = pendingTelemetryPacket;
  havePendingTelemetryPacket = false;
  portEXIT_CRITICAL(&pendingTelemetryMux);

  telemetry.seq = packet.seq;

  for (int i = 0; i < 3; i++) {
    telemetry.potRaw[i] = packet.potRaw[i];
    telemetry.potValue[i] = packet.potValue[i];
    telemetry.pwmDuty[i] = packet.pwmDuty[i];
    telemetry.pwmVoltage[i] = outputVoltage(packet.pwmDuty[i]);
    telemetry.potManual[i] = flagIsSet(packet.flags, i);
  }

  telemetry.switchManual = flagIsSet(packet.flags, 3);
  telemetry.switchRaw = flagIsSet(packet.flags, 4);
  telemetry.switchValue = flagIsSet(packet.flags, 5);
  telemetry.switchVoltage = outputVoltage(packet.switchDuty);
  telemetry.pwm23Changing = flagIsSet(packet.flags, 6);
  telemetry.valid = true;
  telemetry.lastUpdateMs = millis();
  statusLine = "linked esp-now";
  redrawNeeded = true;
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(espNowChannel);

  if (esp_now_init() != ESP_OK) {
    statusLine = "esp-now init failed";
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
      statusLine = "peer add failed";
      Serial.print("ESP-NOW peer add failed: ");
      Serial.println(result);
      return;
    }
  }

  statusLine = "esp-now ready";
  Serial.print("ESP-NOW ready on channel ");
  Serial.print(espNowChannel);
  Serial.print(" MAC ");
  Serial.println(WiFi.macAddress());
}

const char* potCommandName(int row) {
  if (row == ROW_POT1) {
    return "pot1";
  }

  if (row == ROW_POT2) {
    return "pot2";
  }

  return "pot3";
}

uint8_t variableForRow(int row) {
  if (row == ROW_POT1) {
    return variablePot1;
  }

  if (row == ROW_POT2) {
    return variablePot2;
  }

  if (row == ROW_POT3) {
    return variablePot3;
  }

  return variableSwitch;
}

bool isPotRow(int row) {
  return row == ROW_POT1 || row == ROW_POT2 || row == ROW_POT3;
}

bool isEditableRow(int row) {
  return isPotRow(row) || row == ROW_SWITCH;
}

int potIndexForRow(int row) {
  return row - ROW_POT1;
}

void sendCommand(uint8_t command, uint8_t variable, uint16_t value, const String& label) {
  EspNowPacket packet = {};
  packet.magic = packetMagic;
  packet.version = packetVersion;
  packet.kind = packetKindCommand;
  packet.seq = commandSeq++;
  packet.command = command;
  packet.variable = variable;
  packet.commandValue = value;
  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t*)&packet, sizeof(packet));

  if (result == ESP_OK) {
    statusLine = "sent " + label;
  } else {
    statusLine = "send failed " + String(result);
  }

  Serial.println(statusLine);
  redrawNeeded = true;
}

void enableSelectedOverride() {
  if (!isEditableRow(selectedRow)) {
    return;
  }

  if (!telemetry.valid) {
    statusLine = "no telemetry";
    redrawNeeded = true;
    return;
  }

  if (isPotRow(selectedRow)) {
    int potIndex = potIndexForRow(selectedRow);
    sendCommand(commandSet,
                variableForRow(selectedRow),
                telemetry.potValue[potIndex],
                String("SET ") + potCommandName(selectedRow) + " " + telemetry.potValue[potIndex]);
    telemetry.potManual[potIndex] = true;
    return;
  }

  sendCommand(commandSet, variableSwitch, telemetry.switchValue ? 1 : 0, String("SET switch ") + (telemetry.switchValue ? "1" : "0"));
  telemetry.switchManual = true;
}

void toggleSelectedOverride() {
  if (!isEditableRow(selectedRow)) {
    return;
  }

  if (isPotRow(selectedRow)) {
    int potIndex = potIndexForRow(selectedRow);

    if (telemetry.potManual[potIndex]) {
      sendCommand(commandClear, variableForRow(selectedRow), 0, String("CLR ") + potCommandName(selectedRow));
      telemetry.potManual[potIndex] = false;
    } else {
      enableSelectedOverride();
    }

    return;
  }

  if (telemetry.switchManual) {
    sendCommand(commandClear, variableSwitch, 0, "CLR switch");
    telemetry.switchManual = false;
  } else {
    enableSelectedOverride();
  }
}

void clearSelectedOverride() {
  if (!isEditableRow(selectedRow)) {
    return;
  }

  if (isPotRow(selectedRow)) {
    int potIndex = potIndexForRow(selectedRow);
    sendCommand(commandClear, variableForRow(selectedRow), 0, String("CLR ") + potCommandName(selectedRow));
    telemetry.potManual[potIndex] = false;
    return;
  }

  sendCommand(commandClear, variableSwitch, 0, "CLR switch");
  telemetry.switchManual = false;
}

void adjustSelectedValue(int delta) {
  if (!isEditableRow(selectedRow)) {
    return;
  }

  if (!telemetry.valid) {
    statusLine = "no telemetry";
    redrawNeeded = true;
    return;
  }

  if (isPotRow(selectedRow)) {
    int potIndex = potIndexForRow(selectedRow);
    uint16_t nextValue = clampAdc((long)telemetry.potValue[potIndex] + ((long)delta * potStep));
    telemetry.potValue[potIndex] = nextValue;
    telemetry.potManual[potIndex] = true;
    sendCommand(commandSet, variableForRow(selectedRow), nextValue, String("SET ") + potCommandName(selectedRow) + " " + nextValue);
    return;
  }

  telemetry.switchValue = delta > 0;
  telemetry.switchManual = true;
  sendCommand(commandSet, variableSwitch, telemetry.switchValue ? 1 : 0, String("SET switch ") + (telemetry.switchValue ? "1" : "0"));
}

bool hasWordChar(const Keyboard_Class::KeysState& state, char needle) {
  for (char c : state.word) {
    if (c == needle) {
      return true;
    }
  }

  return false;
}

int directionFromKeys(const Keyboard_Class::KeysState& state) {
  if (hasWordChar(state, ';')) {
    return 1;
  }

  if (hasWordChar(state, '.')) {
    return 2;
  }

  if (hasWordChar(state, ',')) {
    return 3;
  }

  if (hasWordChar(state, '/')) {
    return 4;
  }

  return 0;
}

void performDirection(int direction) {
  if (direction == 1) {
    selectedRow = selectedRow == 0 ? ROW_COUNT - 1 : selectedRow - 1;
  } else if (direction == 2) {
    selectedRow = (selectedRow + 1) % ROW_COUNT;
  } else if (direction == 3) {
    adjustSelectedValue(-1);
  } else if (direction == 4) {
    adjustSelectedValue(1);
  }

  redrawNeeded = true;
}

void handleKeyboard() {
  M5Cardputer.update();
  Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();

  if (!M5Cardputer.Keyboard.isPressed()) {
    heldDirection = 0;
    return;
  }

  int direction = directionFromKeys(state);
  unsigned long nowMs = millis();

  if (M5Cardputer.Keyboard.isChange()) {
    if (state.enter) {
      toggleSelectedOverride();
    }

    if (state.del) {
      clearSelectedOverride();
    }

    if (direction != 0) {
      performDirection(direction);
      heldDirection = direction;
      nextRepeatMs = nowMs + firstRepeatDelayMs;
    }

    return;
  }

  if (direction != 0 && direction == heldDirection && nowMs >= nextRepeatMs) {
    performDirection(direction);
    nextRepeatMs = nowMs + repeatDelayMs;
  }
}

String potRowText(int potIndex) {
  String text = "P";
  text += String(potIndex + 1);
  text += " r";
  text += telemetry.potRaw[potIndex];
  text += " e";
  text += telemetry.potValue[potIndex];
  text += " ";
  text += String(telemetry.pwmVoltage[potIndex], 2);
  text += "V ";
  text += telemetry.potManual[potIndex] ? "MAN" : "AUTO";
  return text;
}

String rowText(int row) {
  if (!telemetry.valid) {
    if (row == ROW_LINK) {
      return statusLine;
    }

    return "--";
  }

  if (row == ROW_POT1 || row == ROW_POT2 || row == ROW_POT3) {
    return potRowText(potIndexForRow(row));
  }

  if (row == ROW_SWITCH) {
    String text = "SW raw ";
    text += telemetry.switchRaw ? "ON " : "OFF";
    text += " out ";
    text += telemetry.switchValue ? "ON " : "OFF";
    text += String(telemetry.switchVoltage, 2);
    text += "V ";
    text += telemetry.switchManual ? "MAN" : "AUTO";
    return text;
  }

  if (row == ROW_PWM1 || row == ROW_PWM2 || row == ROW_PWM3) {
    int pwmIndex = row - ROW_PWM1;
    String text = "PWM";
    text += String(pwmIndex + 1);
    text += " duty ";
    text += telemetry.pwmDuty[pwmIndex];
    text += " ";
    text += String(telemetry.pwmVoltage[pwmIndex], 2);
    text += "V";
    return text;
  }

  if (row == ROW_MOVE) {
    String text = "P2/P3 movement ";
    text += telemetry.pwm23Changing ? "YES" : "NO";
    return text;
  }

  unsigned long ageMs = millis() - telemetry.lastUpdateMs;
  String text = "seq ";
  text += telemetry.seq;
  text += " age ";
  text += ageMs;
  text += "ms";
  return text;
}

uint16_t rowColor(int row) {
  if (row == selectedRow) {
    return 0x22B6;
  }

  if (isPotRow(row) && telemetry.potManual[potIndexForRow(row)]) {
    return 0x4208;
  }

  if (row == ROW_SWITCH && telemetry.switchManual) {
    return 0x4208;
  }

  return TFT_BLACK;
}

void drawUi() {
  int width = M5Cardputer.Display.width();
  int height = M5Cardputer.Display.height();
  const int headerHeight = 17;
  const int footerHeight = 12;
  const int rowHeight = 12;
  canvas.fillScreen(TFT_BLACK);
  canvas.fillRect(0, 0, width, headerHeight, 0x39E7);
  canvas.setTextColor(TFT_WHITE, 0x39E7);
  canvas.setCursor(4, 4);
  canvas.print("POT2PWM");

  canvas.setTextColor(telemetry.valid ? TFT_GREEN : TFT_ORANGE, 0x39E7);
  canvas.setCursor(width - 76, 4);
  canvas.print(telemetry.valid ? "LINK" : "NO DATA");

  for (int row = 0; row < ROW_COUNT; row++) {
    int y = headerHeight + row * rowHeight;
    uint16_t background = rowColor(row);
    canvas.fillRect(0, y, width, rowHeight, background);

    if (row == selectedRow) {
      canvas.fillRect(0, y, 3, rowHeight, TFT_CYAN);
    }

    uint16_t textColor = TFT_WHITE;

    if (isEditableRow(row)) {
      bool manual = isPotRow(row) ? telemetry.potManual[potIndexForRow(row)] : telemetry.switchManual;
      textColor = manual ? TFT_ORANGE : TFT_WHITE;
    } else if (row == ROW_MOVE) {
      textColor = telemetry.pwm23Changing ? TFT_YELLOW : TFT_DARKGREY;
    } else if (row == ROW_LINK) {
      textColor = telemetry.valid ? TFT_GREEN : TFT_ORANGE;
    }

    canvas.setTextColor(textColor, background);
    canvas.setCursor(6, y + 2);
    canvas.print(rowText(row));
  }

  canvas.fillRect(0, height - footerHeight, width, footerHeight, 0x2104);
  canvas.setTextColor(TFT_LIGHTGREY, 0x2104);
  canvas.setCursor(4, height - footerHeight + 2);
  canvas.print(statusLine.substring(0, 38));
  canvas.pushSprite(0, 0);
}

void setup() {
  Serial.begin(115200);

  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setBrightness(120);
  canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
  canvas.setTextSize(1);
  canvas.setFont(&fonts::Font0);

  setupEspNow();
  drawUi();
}

void loop() {
  processTelemetry();
  handleKeyboard();

  unsigned long nowMs = millis();
  if (redrawNeeded || nowMs - lastRedrawMs >= redrawIntervalMs) {
    drawUi();
    redrawNeeded = false;
    lastRedrawMs = nowMs;
  }
}
