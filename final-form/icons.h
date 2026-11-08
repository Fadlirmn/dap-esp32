// Icon/chrome drawing, ported from old/Stream32-ESP-Player.ino lines 580-718.
// Uses the global `extern Adafruit_ILI9341 tft` (matches the original's own
// global-tft convention) instead of passing a GFX reference around.
#pragma once
#include <stdint.h>
#include <Arduino.h>

void drawPlayIcon(int cx, int cy, int size, uint16_t color);
void drawPauseIcon(int cx, int cy, int size, uint16_t color);
void drawPrevIcon(int cx, int cy, int size, uint16_t color);
void drawNextIcon(int cx, int cy, int size, uint16_t color);
void drawHeartIcon(int cx, int cy, int r, uint16_t color);
void drawVectorMusicNote(int x, int y, int w, int h, uint16_t color);
void drawBluetoothLogo(int cx, int cy, int size, uint16_t color);
void drawBackIcon(int cx, int cy, int size, uint16_t color);
void drawLibraryIcon(int cx, int cy, int size, uint16_t color);
void drawGearIcon(int cx, int cy, int size, uint16_t color);
void maskCoverCorners(int x, int y, int w, int h, int radius);

// NEW per the dap-ui-design redesign.
void drawPlaylistIcon(int cx, int cy, int size, uint16_t color);   // main-menu "Playlists" cell
void drawSwapIcon(int cx, int cy, int size, uint16_t color);       // main-menu "Mode-switch" cell
void drawBatteryIcon(int x, int y, int w, int h, uint8_t pct);     // header battery glyph (fixed theme colors)
void drawSpeakerIcon(int cx, int cy, int size, bool muted);        // volume overlay (fixed theme colors)

// Visual-fidelity pass vs. dap-ui-design/source/*.dc.html -- exact glyphs transcribed
// from the mockup's SVG paths (see that pass's notes), not freehand approximations.
void drawHamburgerIcon(int cx, int cy, int size, uint16_t color);     // MENU chip: 3 equal bars
void drawListIcon(int cx, int cy, int size, uint16_t color);          // header "list" identity icon: 3 bars, last shorter
void drawBluetoothZigzag(int cx, int cy, int size, uint16_t color);   // BT Receiver/Transmitter identity icon (M7 7l10 10-5 5V2l5 5L7 17)
void drawMusicNoteGlyph(int cx, int cy, int size, uint16_t color);    // BT Transmitter panel's "SD source" icon + mode-select "Local Player" row icon
void drawFlowArrowIcon(int cx, int cy, int size, uint16_t color);     // BT Transmitter panel's "flow direction" icon
void drawRadialSignalIcon(int cx, int cy, int size, uint16_t color);  // Local Player panel's twin "signal" icons
void drawUploadIcon(int cx, int cy, int size, uint16_t color);        // Transfer File header icon + mode-select row icon
void drawEqualizerBars(int cx, int cy, int w, int h, uint16_t color); // "now playing" row glyph (replaces row-index number)

// Leading icon shown before a screen's header title, matching dap-ui-design's
// per-screen identity glyph. Plain uint8_t (not the enum type) so existing call
// sites that still pass a bool (e.g. final-form.ino's drawHeader(title, false))
// keep compiling unchanged -- false/true implicitly convert to 0/1 == HICON_NONE/HICON_LIST.
enum HeaderIcon : uint8_t { HICON_NONE = 0, HICON_LIST, HICON_BACK, HICON_BT, HICON_TRANSFER };
void drawHeader(const char *title, uint8_t headerIcon = HICON_NONE);

// New per the footer-bar structural gap: thin contextual hint bar, bottom of
// every full-screen draw (see layout.h's FOOTER_H).
void drawFooter(const String &hint);

// NEW: accent-colored outline marking the cursor-focused item (encoder/button nav
// has no finger to tap, so every selectable item needs a visible "this one" marker).
// Still used by screens not covered by the dap-ui-design list-row restyle pass
// (Settings sub-screens, BT manager, theme list, reset confirm, main menu grid).
void drawFocusRing(int x, int y, int w, int h);
void drawFocusRingCircle(int cx, int cy, int r);
