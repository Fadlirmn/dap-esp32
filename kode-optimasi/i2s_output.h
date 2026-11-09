// Shared I2S DAC output, used to forward BT Receiver's received A2DP audio.
// Ported from final-form/i2s_output.cpp/.h (minus its Local Player drain
// task -- see i2s_output.cpp -- that mode doesn't exist here).
#pragma once
#include <Arduino.h>

void i2sOutputSetup();  // configures the I2S driver on display_pins.h's PIN_I2S_* pins; call once in setup()
void i2sOutputSetVolume(uint8_t volume0to127);  // software gain applied in i2sWriteStereo16, same 0-127 range as bt_backend's volume
void i2sWriteStereo16(const int16_t *interleavedLR, size_t frameCount);  // blocking write; frameCount = number of L/R sample pairs
