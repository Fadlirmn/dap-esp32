// LVGL 8.x config for ESP32 WROOM-32 (no PSRAM). Only the settings that
// differ from LVGL's defaults are listed — lv_conf_internal.h fills in
// everything else, so this file stays short on purpose.
//
// Arduino IDE: place this file directly in the sketch folder (next to
// kode-optimasi.ino) — the lvgl Arduino library auto-detects it there.
// PlatformIO: place it at the project root (same folder) and it's picked
// up via the default include path; no extra build_flag needed.
#if 1

#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   0

// No filesystem, no GPU, no custom log sink — not used by this UI shell.
#define LV_USE_LOG         0
#define LV_USE_GPU         0
#define LV_USE_FILESYSTEM  0

// Memory pool for all lv_obj/style allocations (not the draw buffer, which
// is a separate static array sized in kode-optimasi.ino). Bumped from 24KB
// after a real OOM crash (confirmed via gdb backtrace: lv_obj_add_style ->
// theme_apply, while building the 16-row Daftar Lagu list) -- one screen's
// worth of widgets should still fit easily inside the no-PSRAM budget
// (58KB/327KB used today), this just gives real margin for the heaviest
// screen instead of being exactly on the edge.
#define LV_MEM_SIZE (32U * 1024U)

// Tick source: kode-optimasi.ino calls lv_tick_inc() itself from guiTask,
// so leave LV_TICK_CUSTOM off.
#define LV_TICK_CUSTOM 0

// Fonts used by ui_theme.h (title=18 bold-weight unavailable in the built-in
// set, see ponytail note in ui_theme.h; body=14; footer/meta hints=12, the
// closest built-in size to the mockups' 10-11px micro text).
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_DEFAULT       &lv_font_montserrat_14

// Built-in play/pause/prev/next/etc. glyphs (LV_SYMBOL_*), no bitmap assets.
#define LV_USE_IMG  1
#define LV_USE_LINE 1
#define LV_USE_ARC  1
#define LV_USE_BAR  1

#endif
