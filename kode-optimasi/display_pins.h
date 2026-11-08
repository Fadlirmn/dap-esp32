#pragma once
// Shared SPI bus (TFT + SD + touch), same physical wiring as final-form/pins.h.
// Standalone copy on purpose — this folder does not #include across projects.

#define PIN_SPI_SCK   18
#define PIN_SPI_MOSI  23
#define PIN_SPI_MISO  19

#define PIN_TFT_CS    17
#define PIN_TFT_DC    27
#define PIN_TFT_RST   33

#define SCREEN_W      320
#define SCREEN_H      240
