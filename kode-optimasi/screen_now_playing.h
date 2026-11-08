#pragma once
// L1 "Now Playing" (Local Player). Public setters let a future integration
// pass feed real track/progress/battery data instead of the demo values this
// standalone module seeds itself with; they no-op safely if this screen isn't
// the active one (guarded against use-after-free via an LV_EVENT_DELETE hook).
#include "lvgl.h"

void screen_now_playing_create(lv_obj_t *parent);

void now_playing_set_track_info(const char *title, const char *subtitle);
void now_playing_set_progress(uint8_t percent, const char *elapsed, const char *total);
void now_playing_set_playing(bool is_playing);
void now_playing_set_battery(int percent);
bool now_playing_is_playing(void); // so the elapsed-time tick (demo_cycle.cpp) can stop advancing while paused
void now_playing_tick_elapsed(void); // advances elapsed by 1s against the CURRENT track's own duration
