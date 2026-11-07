// Screen geometry, ported verbatim from old/Stream32-ESP-Player.ino (portrait 240x320)
// so every ported screen-drawing coordinate lines up unchanged.
#pragma once

constexpr int SCREEN_W = 240;
constexpr int SCREEN_H = 320;

constexpr int HEADER_H = 35;
constexpr int MENU_BTN_W = 105;
constexpr int MENU_BTN_H = 70;
constexpr int MENU_BTN_X_LEFT = 10;
constexpr int MENU_BTN_X_RIGHT = 125;
constexpr int MENU_BTN_Y[3] = {48, 128, 208};
constexpr int NAV_Y = 280;
constexpr int NAV_H = 40;

constexpr int PL_ART_X = 65, PL_ART_Y = 38, PL_ART_W = 110, PL_ART_H = 110;

constexpr int PL_INFO_X = 20;
constexpr int PL_TITLE_Y = 153;
constexpr int PL_ARTIST_Y = 174;
constexpr int PL_HEART_CX = 220, PL_HEART_CY = 158, PL_HEART_R = 9;

constexpr int PL_PROGRESS_X = 20, PL_PROGRESS_Y = 191, PL_PROGRESS_W = 200, PL_PROGRESS_H = 8;

constexpr int PL_BTN_Y = 207, PL_BTN_H = 32;
constexpr int PL_PREV_X = 25, PL_PREV_W = 45;
constexpr int PL_PLAY_X = 97, PL_PLAY_W = 46;
constexpr int PL_NEXT_X = 170, PL_NEXT_W = 45;

constexpr int PL_VOL_Y = 245, PL_VOL_H = 25;
constexpr int PL_VOLDOWN_X = 20, PL_VOLDOWN_W = 40;
constexpr int PL_VOLUP_X = 180, PL_VOLUP_W = 40;
constexpr int PL_VOLTEXT_X = 70, PL_VOLTEXT_W = 100;

constexpr int LIST_ITEMS_PER_PAGE = 6;
constexpr int LIST_ITEM_H = 26;
constexpr int LIST_ITEM_Y_START = 45;
constexpr int LIST_ITEM_GAP = 2;
