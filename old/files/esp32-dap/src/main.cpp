#include <Arduino.h>
#include "config.h"
#include "common.h"
#include "ui.h"
#include "modes.h"

static uint8_t mode;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);   // cegah SD mengganggu bus SPI saat layar init
  inputBegin();
  batteryBegin();
  uiBegin();

  mode = loadMode();
  uiSplash(modeName(mode));
  delay(600);

  switch (mode) {
    case MODE_RECEIVER:    receiverSetup();    break;
    case MODE_TRANSMITTER: transmitterSetup(); break;
    case MODE_TRANSFER:    transferSetup();    break;
    default:               localSetup();       break;
  }
}

void loop() {
  static uint32_t batAt = 0, iconAt = 0;
  uint32_t now = millis();
  if (now - batAt > 1000)   { batAt = now;  batteryUpdate(); }
  if (now - iconAt > 30000) { iconAt = now; uiBatteryIcon(); }

  switch (mode) {
    case MODE_RECEIVER:    receiverLoop();    break;
    case MODE_TRANSMITTER: transmitterLoop(); break;
    case MODE_TRANSFER:    transferLoop();    break;
    default:               localLoop();       break;
  }
}
