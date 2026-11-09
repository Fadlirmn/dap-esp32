#pragma once
// Pin map for the real hardware (was final-form/pins.h, merged in when
// final-form was retired — kode-optimasi is now the only firmware project).

#define ENABLE_TOUCH 1   // 0 = encoder+buttons only, drop XPT2046 entirely

#define PIN_SPI_SCK   18
#define PIN_SPI_MOSI  23
#define PIN_SPI_MISO  19

#define PIN_TFT_CS    17
#define PIN_TFT_DC    27
#define PIN_TFT_RST   33

#define PIN_SD_CS     4

#define PIN_TOUCH_CS  21
#define PIN_TOUCH_IRQ 34

// Rotary encoder (KY-040)
#define PIN_ENC_CLK 13
#define PIN_ENC_DT  14
#define PIN_ENC_SW  16

// Top buttons (active LOW to GND)
#define PIN_BTN_PREV 32
#define PIN_BTN_NEXT 36
#define PIN_BTN_MENU 39

// I2S DAC (BT Receiver forwarded audio output)
#define PIN_I2S_BCLK 25
#define PIN_I2S_LRC  26
#define PIN_I2S_DOUT 22

#define SCREEN_W      320
#define SCREEN_H      240
