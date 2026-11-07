// Screen geometry: landscape 320x240 (rotated from the original portrait
// 240x320 port). Bottom nav bar is gone (it was touch-only decoration, dead
// weight on the encoder+button path) -- that recovers most of the height
// lost to rotation, so every screen keeps its header + content without a
// from-scratch redesign.
#pragma once

constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;

constexpr int HEADER_H = 28;

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

// Mode select screen (STATE_MODE_SELECT): list of 4 modes + Kembali.
constexpr int MODE_LIST_Y_START = 36;
constexpr int MODE_LIST_ITEM_H = 30;
constexpr int MODE_LIST_GAP = 4;

// Floating auto-hiding volume overlay: pill bar centered near the bottom,
// drawn over whatever screen is active while the encoder is being turned.
constexpr int VOL_OVERLAY_W = 220, VOL_OVERLAY_H = 40;
constexpr int VOL_OVERLAY_X = (SCREEN_W - VOL_OVERLAY_W) / 2;
constexpr int VOL_OVERLAY_Y = SCREEN_H - VOL_OVERLAY_H - 14;

// Player screen: cover art left, title/artist/progress/transport/volume
// stacked on the right and below.
constexpr int PL_ART_X = 14, PL_ART_Y = 34, PL_ART_W = 100, PL_ART_H = 100;

constexpr int PL_INFO_X = 128;
constexpr int PL_TITLE_Y = 40;
constexpr int PL_ARTIST_Y = 66;
constexpr int PL_HEART_CX = 300, PL_HEART_CY = 46, PL_HEART_R = 9;

constexpr int PL_PROGRESS_X = 14, PL_PROGRESS_Y = 142, PL_PROGRESS_W = 292, PL_PROGRESS_H = 8;

constexpr int PL_BTN_CY = 180, PL_BTN_R = 20;
constexpr int PL_PREV_CX = 110;
constexpr int PL_PLAY_CX = 160;
constexpr int PL_NEXT_CX = 210;

constexpr int PL_VOL_Y = 212, PL_VOL_H = 22;
constexpr int PL_VOLDOWN_X = 14, PL_VOLDOWN_W = 36;
constexpr int PL_VOLUP_X = 270, PL_VOLUP_W = 36;
constexpr int PL_VOLTEXT_X = 60, PL_VOLTEXT_W = 200;

constexpr int LIST_ITEMS_PER_PAGE = 6;
constexpr int LIST_ITEM_H = 24;
constexpr int LIST_ITEM_Y_START = 34;
constexpr int LIST_ITEM_GAP = 2;
