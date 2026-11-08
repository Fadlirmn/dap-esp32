// A2DP sink (phone -> wired IEM via i2s_output.h), for MODE_BT_RECEIVER.
// Mirrors bluetooth.h's naming/shape but is a separate module: the source
// (bluetooth.h/.cpp, BT Transmitter mode) and sink are mutually exclusive
// per boot, never linked into the same running mode.
#pragma once
#include <Arduino.h>

extern volatile bool btrx_connected;
extern volatile bool btrx_playing;
extern String btrx_deviceName;  // connected phone's name once known, else "No device"

void btReceiverSetup();                        // call once in setup() when operatingMode == MODE_BT_RECEIVER
void btReceiverSetVolume(uint8_t volume0to127);  // called from nav.cpp's encoder-rotate handler

// AVRCP passthrough transport controls: this device is the sink, so
// "play/pause/next/prev" means telling the connected PHONE to do that
// (there's no local track to control) -- the dap-ui-landscape mockup shows
// these buttons on the BT Receiver screen, nav.cpp wires them to these.
void btReceiverPlayPause();
void btReceiverNext();
void btReceiverPrev();
