// Screen geometry: landscape 320x240 (rotated from the original portrait
// 240x320 port). Bottom nav bar is gone (it was touch-only decoration, dead
// weight on the encoder+button path) -- that recovers most of the height
// lost to rotation, so every screen keeps its header + content without a
// from-scratch redesign.
#pragma once

constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;

constexpr int HEADER_H = 28;
constexpr int FOOTER_H = 28;  // hint bar, per dap-ui-design's footer -- was missing entirely before

// Main menu: 2x3 grid of circular icon buttons (Player/Tracks/Playlists/Likes/
// Settings/Mode-switch), no text labels -- smartwatch app-grid style. Only
// shown for MODE_LOCAL_PLAYER and MODE_BT_TRANSMITTER (nav.h); the other two
// modes boot straight into their own dedicated screen.
constexpr int MENU_CELL_CX[3] = {54, 160, 266};
constexpr int MENU_CELL_CY[2] = {88, 178};
constexpr int MENU_CELL_R = 34;

// Dummy battery %: no ADC pin wired for real battery sensing (pins.h has
// none), so the header just shows this constant. Upgrade path: replace with
// a real analogRead() once a sense pin + divider exist.
constexpr uint8_t DUMMY_BATTERY_PCT = 78;

// Mode select screen (STATE_MODE_SELECT): list of 4 modes + Kembali. Rows grew
// from a single text line to icon-circle + title + subtitle (dap-ui-design),
// so Y_START/ITEM_H/GAP changed to fit a 32x32 icon circle per row while still
// landing all 5 rows above the new FOOTER_H band -- nav.cpp doesn't hardcode
// this screen's row geometry (grep-verified), so these are safe to retune.
constexpr int MODE_LIST_Y_START = 32;
constexpr int MODE_LIST_ITEM_H = 32;
constexpr int MODE_LIST_GAP = 3;

// Floating auto-hiding volume overlay: near-pill bar near the TOP of the
// content area (dap-ui-design puts it just under the header, not bottom-center).
constexpr int VOL_OVERLAY_X = 20, VOL_OVERLAY_W = SCREEN_W - 40, VOL_OVERLAY_H = 44;
constexpr int VOL_OVERLAY_Y = HEADER_H + 6;

// Player-type screens (drawPlayerScreen/drawBTReceiverScreen): the mockup has
// no album art, just a wide mode-identity panel near the top (bg/content vary
// by OperatingMode, see screens.cpp's drawNowPlayingPanel) followed by an
// optional connection-status chip, title/subtitle, progress, transport.
// PL_ART_* kept (append-only) even though drawCoverArt() is now unused by
// drawPlayerScreen -- the function/declaration stays, just uncalled there.
constexpr int PL_ART_X = 14, PL_ART_Y = 34, PL_ART_W = 100, PL_ART_H = 100;

constexpr int PL_PANEL_X = 14, PL_PANEL_Y = HEADER_H + 4, PL_PANEL_W = SCREEN_W - 28, PL_PANEL_H = 50;
constexpr int PL_CHIP_Y = 88, PL_CHIP_H = 22;  // BT Transmitter/Receiver connection chip only

constexpr int PL_INFO_X = 14;   // was 128 -- the cover-art column is gone, text now spans full width
constexpr int PL_TITLE_Y = 118;
constexpr int PL_ARTIST_Y = 135;
constexpr int PL_HEART_CX = 300, PL_HEART_CY = 121, PL_HEART_R = 9;

constexpr int PL_PROGRESS_X = 14, PL_PROGRESS_Y = 150, PL_PROGRESS_W = 292, PL_PROGRESS_H = 6;

constexpr int PL_BTN_CY = 188, PL_BTN_R = 20;
constexpr int PL_PREV_CX = 110;
constexpr int PL_PLAY_CX = 160;
constexpr int PL_NEXT_CX = 210;

constexpr int PL_VOL_Y = 212, PL_VOL_H = 22;
constexpr int PL_VOLDOWN_X = 14, PL_VOLDOWN_W = 36;
constexpr int PL_VOLUP_X = 270, PL_VOLUP_W = 36;
constexpr int PL_VOLTEXT_X = 60, PL_VOLTEXT_W = 200;

// Track/Likes/Playlists/PlaylistDetail rows: no-card plain rows (dap-ui-design),
// tall enough for a title + secondary line. nav.cpp's STATE_TRACK_LIST/
// STATE_LIKES_LIST touch hit-test reads these same symbolic constants (not
// hardcoded pixels), so retuning them here stays safe for touch too.
constexpr int LIST_ITEMS_PER_PAGE = 6;
constexpr int LIST_ITEM_H = 28;
constexpr int LIST_ITEM_Y_START = 32;
constexpr int LIST_ITEM_GAP = 2;
