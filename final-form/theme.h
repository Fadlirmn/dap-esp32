// Color theme, ported verbatim from old/Stream32-ESP-Player.ino. tft.color565() works
// unchanged: Adafruit_ILI9341 inherits it from Adafruit_SPITFT, same call signature
// TFT_eSPI used.
#pragma once
#include <stdint.h>

extern uint16_t COLOR_BG, COLOR_PANEL, COLOR_PANEL_BORDER, COLOR_ACCENT, COLOR_ACCENT_DIM,
    COLOR_TEXT_PRIMARY, COLOR_TEXT_SECONDARY, COLOR_HEART_ACTIVE, COLOR_HEART_INACTIVE,
    COLOR_TRACK_BG, COLOR_BT_CONNECTED, COLOR_BT_DISCONNECTED;

// Status/semantic colors, fixed regardless of the user's accent theme choice
// (dap-ui-design's Material dark palette) -- used by the new mode-switch,
// BT Receiver and Transfer File screens.
extern uint16_t COLOR_BT_PANEL, COLOR_BT_PANEL_TEXT, COLOR_WARNING, COLOR_SUCCESS,
    COLOR_CHIP_BG, COLOR_CHIP_TEXT;

// Visual-fidelity pass vs. dap-ui-design/source/*.dc.html: the mockup uses a
// *second*, lighter neutral shade for buttons/rows/status-chips (#272A30),
// distinct from the darker header/footer COLOR_PANEL (#1C2026) -- collapsing
// them into one shade was the main "doesn't look like the design" gap.
extern uint16_t COLOR_BUTTON_BG;      // #272A30 -- transport buttons, mode-select icon circles, status chips
extern uint16_t COLOR_ACCENT_TEXT;    // #003062 -- icon/text drawn on top of an COLOR_ACCENT-filled shape (e.g. the play button)
extern uint16_t COLOR_OVERLAY_TRACK;  // #4A5059 -- volume overlay's unfilled track (player progress keeps COLOR_TRACK_BG)
extern uint16_t COLOR_ERROR;          // #FFB4AB -- muted volume / low-battery / error states

// NEW: Local Player's top panel is filled with COLOR_ACCENT_DIM, and the mockup
// draws its two "signal" icons in a light tint on top of that -- not plain
// white/COLOR_TEXT_PRIMARY, and not quite COLOR_CHIP_TEXT either.
extern uint16_t COLOR_ON_ACCENT_DIM;  // #D6E3FF -- icon color on top of a COLOR_ACCENT_DIM-filled panel

extern uint8_t themeIndex;

void initColorTheme();
void applyTheme(uint8_t theme);
