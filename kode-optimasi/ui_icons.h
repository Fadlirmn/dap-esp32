#pragma once
// Shared icon widgets. LVGL's built-in LV_SYMBOL_* glyph font already covers
// play/pause/prev/next/shuffle/loop/bluetooth/battery/ok/volume/mute/upload/
// list/bars/left/warning/audio/wifi — so those are just styled lv_label's
// (icon_symbol below), not hand-drawn vector art. Only the handful of shapes
// LVGL has no built-in glyph for (battery+badge combo, connected checkmark
// badge, equalizer "now playing" bars) get a small composite widget here.
// Nothing in this module touches a bitmap/image asset.
#include "lvgl.h"

// Plain glyph label, e.g. icon_symbol(parent, LV_SYMBOL_PLAY, COLOR_ACCENT_TEXT, FONT_BODY).
lv_obj_t *icon_symbol(lv_obj_t *parent, const char *symbol, lv_color_t color, const lv_font_t *font);

// Battery glyph (auto-picks LV_SYMBOL_BATTERY_FULL..EMPTY by percent, red under 15%) + "NN%" label.
lv_obj_t *icon_battery(lv_obj_t *parent, int percent);

// Small green circle + white checkmark, as used on BT Receiver/Transmitter hero cards.
lv_obj_t *icon_connected_badge(lv_obj_t *parent);

// 3 small vertical bars (varying height) — the "now playing" marker shown
// instead of a track index number in list rows.
lv_obj_t *icon_equalizer_bars(lv_obj_t *parent, lv_color_t color);
