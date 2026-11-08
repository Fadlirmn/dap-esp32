#pragma once
// Shared floating volume HUD (covers L9 "Volume berubah" + L10 "Volume
// 0/senyap" — same widget, state driven by percent). Lives on lv_layer_top(),
// base-screen-agnostic, auto-hides after 2s. Call from guiTask only.
#include "lvgl.h"

// percent <= 0 renders the muted/red state (L10); >0 the normal/green state (L9).
void overlay_volume_show(int percent);

// Real, persistent volume level (not a fixed demo number) -- clamps to
// [0,100], updates the one module-level volume state, and shows it via
// overlay_volume_show(). delta<0 lowers, delta>0 raises. Wire this to a real
// input (sim: mouse wheel; hardware: volume buttons) instead of a timer.
void overlay_volume_adjust(int delta);
