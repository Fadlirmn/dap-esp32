// UI state machine + cursor state. Screens (screens.cpp) read this to know what
// to draw and what to focus-ring; input.cpp/touch.cpp write it in response to events.
#pragma once
#include <Arduino.h>
#include "pins.h"  // ENABLE_TOUCH must be defined before the #if ENABLE_TOUCH guards below

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
    STATE_THEME_SETTINGS,
    STATE_PLAYLISTS,
    STATE_PLAYLIST_DETAIL,
    STATE_MODE_SELECT,      // mockup's "MENU" screen: pick which operating mode is active
    STATE_MODE_SWITCHING,   // "Beralih mode" transition screen shown just before ESP.restart()
    STATE_BT_RECEIVER,      // phone audio -> wired IEM (A2DP sink)
    STATE_TRANSFER_FILE     // WiFi AP + HTTP upload onto the SD card
};
extern UIState currentState;

// One mode is active per boot; switching modes saves the choice and restarts
// (see selectOperatingMode()). BT_TRANSMITTER is today's whole existing app
// (SD -> A2DP source -> headset) under its mockup name, so it reuses
// STATE_MENU/STATE_PLAYER/etc. unchanged; LOCAL_PLAYER reuses the same UI but
// outputs over the I2S DAC (audio_decoder.cpp) instead of A2DP.
enum OperatingMode : uint8_t { MODE_LOCAL_PLAYER, MODE_BT_RECEIVER, MODE_BT_TRANSMITTER, MODE_TRANSFER_FILE };
extern OperatingMode operatingMode;  // set from Preferences("mode_pref") before setup() inits anything else

void loadOperatingMode();                 // call first in setup(), before any subsystem init
UIState homeStateForMode();                // STATE_MENU, or the active mode's own dedicated screen
void enterModeSelect();                   // MENU long-press, from anywhere: open STATE_MODE_SELECT
void selectOperatingMode(OperatingMode m);  // confirm: persists pref, shows STATE_MODE_SWITCHING, ESP.restart()
extern OperatingMode switchingToMode;       // set by selectOperatingMode() just before STATE_MODE_SWITCHING draws, so screens.cpp knows which mode it's switching to

// Low-battery dialog + SD-error toast (dap-ui-landscape's L_Overlays screen).
// Both paint on top of whatever screen is current, same architecture as the
// volume overlay -- drawCurrentStateUI() (screens.cpp) draws them after the
// normal per-state dispatch, every redraw, as long as the flag is set.
// No real battery sensor exists (DUMMY_BATTERY_PCT in layout.h is a fixed
// constant, see pins.h), so checkLowBattery() is honest about being a no-op
// until real sensing exists: it only ever fires if that constant itself is
// below the threshold, never from a fabricated "reading".
constexpr uint8_t LOW_BATTERY_THRESHOLD_PCT = 15;
extern volatile bool lowBatteryOverlayActive;
extern volatile bool sdErrorOverlayActive;
void checkLowBattery();  // call every loop() tick
void checkSDHealth();    // call every loop() tick (throttled internally): periodic cheap SD.exists() probe

// Floating auto-hiding volume overlay: any screen can show it over itself
// while the encoder is being turned; checkVolumeOverlayTimeout() (called every
// loop() tick, like checkScreenTimeout()) clears it ~1.5s after rotation stops.
constexpr unsigned long VOLUME_OVERLAY_MS = 1500;
extern unsigned long lastVolumeChangeAt;  // 0 = overlay hidden
void checkVolumeOverlayTimeout();

// Single shared cursor: only one screen is visible at a time, so one index
// (reset on state entry) is enough to track "which item is focused" for every
// list/grid/button-row screen (menu, settings list, theme list, track list,
// BT manager CONNECT/SCAN, reset CANCEL/RESET, BT scan results, screen-settings
// timeout chips -- the Brightness row is gone, so no two-level focus is needed).
extern int focusIndex;

extern int trackList_currentPage;
extern int likesList_currentPage;
extern int playlist_selectedIndex;  // which playlist STATE_PLAYLIST_DETAIL is showing (index into playlist.h's playlistNames)

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
void handleMenuButton();               // short press: open menu / back (reproduces original per-screen back targets)
void handleMenuLongPress();            // long press: open STATE_MODE_SELECT from anywhere

#if ENABLE_TOUCH
void processGlobalTouch(int x, int y);  // optional secondary input path, same actions
#endif
