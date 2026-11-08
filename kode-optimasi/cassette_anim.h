#pragma once
// Vector spinning-reel decoration for the Now Playing hero card. Each reel is
// one lv_line "spoke" rotated via LVGL's style transform (no canvas, no GIF,
// no bitmap frames). Stepped by whatever cadence the caller's lv_timer_handler
// runs at (kode-optimasi.ino drives guiTask at 66ms / 15 FPS).
#include "lvgl.h"

// Creates both reels + the tape window as children of `card` (the hero card obj).
void cassette_anim_create(lv_obj_t *card, lv_coord_t card_w, lv_coord_t card_h);

// Pauses (false) or (re)starts (true) the spin — called when playback toggles.
void cassette_anim_set_spinning(bool spinning);
