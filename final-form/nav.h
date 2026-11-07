// UI state machine + cursor state. Screens (screens.cpp) read this to know what
// to draw and what to focus-ring; input.cpp/touch.cpp write it in response to events.
#pragma once
#include <Arduino.h>

enum UIState {
    STATE_MENU,
    STATE_PLAYER,
    STATE_TRACK_LIST,
    STATE_LIKES_LIST,
    STATE_BT_MANAGER,
    STATE_SETTINGS,
    STATE_BT_SCAN,
    STATE_SCREEN_SETTINGS,
    STATE_PLAYBACK_SETTINGS,
    STATE_INFO,
    STATE_RESET_CONFIRM,
    STATE_THEME_SETTINGS
};
extern UIState currentState;

// Single shared cursor: only one screen is visible at a time, so one index
// (reset on state entry) is enough to track "which item is focused" for every
// list/grid/button-row screen (menu, settings list, theme list, track list,
// BT manager CONNECT/SCAN, reset CANCEL/RESET, BT scan results, screen-settings
// timeout chips -- the Brightness row is gone, so no two-level focus is needed).
extern int focusIndex;

extern int trackList_currentPage;
extern int likesList_currentPage;

// Screen settings. Brightness dropped: backlight is hard-wired to 3V3, no PWM pin
// available (see pins.h). Timeout is the only screen setting left.
extern uint8_t screenTimeoutMinutes;

// Idle screen lock (ported concept from the original's screenLocked/lastScreenActivity,
// generalized to wake on any input source, not just touch).
extern bool screenLocked;
void checkScreenTimeout();  // call every loop() tick

void enterState(UIState newState);  // sets currentState, resets focusIndex, redraws

void handleEncoderRotate(int delta);   // always volume
void handleEncoderClick();             // select/open focused item; Player = play/pause
void handleEncoderDoubleClick();       // Player = next track
void handleEncoderLongPress();         // Player = toggle like
void handlePrevButton();               // move focus up / Player = prev track
void handleNextButton();               // move focus down / Player = next track
void handleMenuButton();               // open menu / back (reproduces original per-screen back targets)

#if ENABLE_TOUCH
void processGlobalTouch(int x, int y);  // optional secondary input path, same actions
#endif
