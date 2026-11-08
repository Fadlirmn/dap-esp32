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
    SCREEN_COUNT
} ScreenId;

void screen_manager_init(ScreenId initial);
void screen_manager_show(ScreenId id);
ScreenId screen_manager_current(void);
