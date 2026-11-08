// Copy this file's contents into <Arduino/libraries>/TFT_eSPI/User_Setup.h
// (Arduino IDE), or pass the same defines as PlatformIO build_flags instead
// of copying a file — see README.md. Matches display_pins.h.
//
// Panel is ILI9341 on the actual hardware this was ported from
// (final-form/pins.h). Swap ILI9341_DRIVER -> ST7789_DRIVER below if your
// panel is ST7789.

#define USER_SETUP_LOADED

#define ILI9341_DRIVER

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   17
#define TFT_DC   27
#define TFT_RST  33

#define LOAD_GLCD
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
