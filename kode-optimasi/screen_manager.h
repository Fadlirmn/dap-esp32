#pragma once
// Lazy screen lifecycle: one persistent root screen is loaded once; each
// "screen" is really just that root's children, rebuilt from scratch per
// show() (old children freed via lv_obj_clean before the new ones are
// built), so peak memory is always one screen's worth, never two. Must only
// be called from guiTask (the one task that owns LVGL).
#include "lvgl.h"

typedef enum {
    SCREEN_NOW_PLAYING,
    SCREEN_LIBRARY,
    SCREEN_MENU,
    SCREEN_RECEIVER,
    SCREEN_TRANSMITTER,
    SCREEN_TRANSFER,
    SCREEN_SWITCH,
    SCREEN_PLAYLISTS,
    SCREEN_PLAYLIST_DETAIL,
    SCREEN_BT_DEVICES_RX, // "pilih perangkat" before BT Receiver -- new, not one of the original 12 boards
    SCREEN_BT_DEVICES_TX, // same screen/logic, reused for BT Transmitter's device list
    SCREEN_EQUALIZER,        // EQ preset picker -- new, not one of the original 12 boards
    SCREEN_EQUALIZER_CUSTOM, // band editor for the active preset, reached from SCREEN_EQUALIZER's gear button
    SCREEN_VOLUME,           // volume settings, reached from the header's volume badge (opposite the home badge)
    SCREEN_COUNT
} ScreenId;

void screen_manager_init(ScreenId initial);
void screen_manager_show(ScreenId id);
ScreenId screen_manager_current(void);

// The current screen's focus ring for the physical PREV/NEXT/ENC_SW indev
// (see input.cpp) -- cleared automatically on every screen_manager_show(),
// so each screen_*_create() just adds its own clickable buttons/rows to it
// right after creating them, in the order PREV/NEXT should tab through.
lv_group_t *screen_manager_group(void);
