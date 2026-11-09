#pragma once
#include "lvgl.h"
void screen_transfer_create(lv_obj_t *parent);

// Call every ~1s while this screen is showing (see demo_cycle.cpp) to refresh
// the files-received/upload-progress labels from wifi_transfer.h's globals.
void screen_transfer_tick(void);
