// Pin map for the real hardware. Core pins (TFT/encoder/buttons) come verbatim from
// old/files/test_input.ino; SD CS and touch CS/IRQ come from old/files/plan.md (same board,
// shared SPI bus) since test_input.ino never wired those.
#pragma once

#define ENABLE_TOUCH 1   // 0 = encoder+buttons only, drop XPT2046 entirely

// TFT (ILI9341)
#define PIN_TFT_CS   17
#define PIN_TFT_DC   27
#define PIN_TFT_RST  33

// Shared SPI bus (TFT + SD + touch)
#define PIN_SPI_SCK  18
#define PIN_SPI_MOSI 23
#define PIN_SPI_MISO 19

#define PIN_SD_CS    4

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

// Backlight is wired straight to 3V3 (no PWM pin available: GPIO32 is taken by PREV).

// I2S DAC (Local Player wired IEM output). Proposed assignment -- none of
// these clash with the pins above; correct here if your module differs.
#define PIN_I2S_BCLK 25
#define PIN_I2S_LRC  26
#define PIN_I2S_DOUT 22
