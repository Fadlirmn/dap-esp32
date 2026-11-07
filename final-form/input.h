// Encoder + button polling. ISR/volume logic ported from old/files/test_input.ino;
// click/double-click/long-press and button hold-repeat are new (neither source
// file needed them: test_input.ino only counted clicks, Stream32 was touch-only).
#pragma once
#include <Arduino.h>

void inputSetup();   // pinMode + attachInterrupt, call once from setup()
void inputPoll();    // call every loop() tick; fires the nav.h handlers directly
