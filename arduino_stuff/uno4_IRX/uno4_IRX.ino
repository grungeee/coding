#include <Arduino_LED_Matrix.h>
#include <IRremote.hpp>

// KY-022 OUT -> D2 by default. Change this if you wired it elsewhere.
constexpr uint8_t IR_RECEIVE_PIN = 2;
constexpr unsigned long RECEIVE_ICON_HOLD_MS = 900;

ArduinoLEDMatrix matrix;

unsigned long lastReceiveMs = 0;

uint8_t idleFrame[8][12] = {
  {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0},
  {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0},
  {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0},
  {0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0}
};

uint8_t receivedFrame[8][12] = {
  {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0},
  {0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0},
  {0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0},
  {0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0},
  {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0}
};

void showIdle() {
  matrix.renderBitmap(idleFrame, 8, 12);
}

void showReceived() {
  matrix.renderBitmap(receivedFrame, 8, 12);
  lastReceiveMs = millis();
}

void printDecodedSignal() {
  Serial.print(F("Protocol: "));
  Serial.println(IrReceiver.getProtocolString());

  IrReceiver.printIRResultShort(&Serial);
  IrReceiver.printIRSendUsage(&Serial);

  if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
    Serial.println(F("Repeat frame detected."));
  }

  if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
    Serial.println(F("Unknown protocol. Raw timing dump:"));
    IrReceiver.printIRResultRawFormatted(&Serial, true);
  }

  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  matrix.begin();
  showIdle();

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

  Serial.println(F("UNO R4 WiFi IR protocol detector"));
  Serial.print(F("IR receiver pin: D"));
  Serial.println(IR_RECEIVE_PIN);
  Serial.print(F("Listening for: "));
  printActiveIRProtocols(&Serial);
  Serial.println();
  Serial.println(F("Open Serial Monitor at 115200 baud, then press remote buttons."));
  Serial.println();
}

void loop() {
  if (IrReceiver.decode()) {
    showReceived();
    printDecodedSignal();
    IrReceiver.resume();
  }

  if ((lastReceiveMs != 0) && (millis() - lastReceiveMs >= RECEIVE_ICON_HOLD_MS)) {
    showIdle();
    lastReceiveMs = 0;
  }
}
