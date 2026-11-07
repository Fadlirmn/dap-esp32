// Shared I2S DAC output, used by both Local Player (decoded SD audio,
// audio_decoder.cpp) and BT Receiver (received A2DP audio, bt_receiver.cpp)
// -- only one of those runs per boot (see nav.h's OperatingMode), so one
// output module covers both instead of duplicating the I2S driver setup.
#pragma once
#include <Arduino.h>

void i2sOutputSetup();  // configures the I2S driver on pins.h's PIN_I2S_* pins; call once in setup() when operatingMode is MODE_LOCAL_PLAYER or MODE_BT_RECEIVER
void i2sOutputSetVolume(uint8_t volume0to127);  // software gain applied in i2sWriteStereo16, same 0-127 range as bt_volume
void i2sWriteStereo16(const int16_t *interleavedLR, size_t frameCount);  // blocking write; frameCount = number of L/R sample pairs
