// Optional secondary input: XPT2046 resistive touch, shared SPI bus (unlike the
// original's dedicated HSPI bus -- this hardware's touch pins share SCK/MOSI/MISO
// with the TFT/SD per old/files/plan.md). Compiled only when pins.h sets ENABLE_TOUCH.
// Ported from the touch block in old/Stream32-ESP-Player.ino's loop() (lines 1807-1845).
#pragma once
#include "pins.h"

#if ENABLE_TOUCH
void touchSetup();
void touchPoll();  // call every loop() tick; dispatches into nav.h's processGlobalTouch
#endif
