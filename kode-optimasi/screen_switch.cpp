// L8 "Beralih mode" — chrome-less transient screen shown while the ESP32
// restarts into a new operating mode. Meant to auto-advance via a timer in
// the caller (kode-optimasi.ino's demo cycle); no input handling of its own.
#include "screen_switch.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"

void screen_switch_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;

    lv_obj_t *row = lv_obj_create(scr);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, SCREEN_W - 56, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 20, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(row);

    lv_obj_t *tile = ui_card_create(row, 0, 0, 84, 84, 28, COLOR_BT_HERO);
    lv_obj_center(icon_symbol(tile, LV_SYMBOL_BLUETOOTH, COLOR_BT_HERO_ICON, FONT_TITLE));

    lv_obj_t *text_col = lv_obj_create(row);
    lv_obj_remove_style_all(text_col);
    lv_obj_set_size(text_col, 150, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(text_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(text_col);
    lv_label_set_text(label, "Beralih ke");
    lv_obj_set_style_text_color(label, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(label, FONT_SMALL, 0);

    lv_obj_t *target = lv_label_create(text_col);
#ifdef KODE_OPTIMASI_SIM
    lv_label_set_text(target, "BT Receiver");
#else
    // No real mode-switch state is passed into this screen yet (it isn't
    // even reachable from the UI right now -- see PRD §5.9) -- a future
    // pass wires the actual target mode name in here.
    lv_label_set_text(target, "-");
#endif
    lv_obj_set_style_text_color(target, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(target, FONT_TITLE, 0);

    lv_obj_t *bar = lv_bar_create(text_col);
    lv_obj_set_size(bar, 150, 4);
    lv_obj_set_style_pad_top(bar, 14, 0);
    lv_obj_set_style_bg_color(bar, COLOR_TRACK, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_bar_set_range(bar, 0, 100);
#ifdef KODE_OPTIMASI_SIM
    lv_bar_set_value(bar, 35, LV_ANIM_OFF);
#else
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
#endif

    lv_obj_t *caption = lv_label_create(text_col);
    lv_label_set_long_mode(caption, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(caption, 150);
    lv_label_set_text(caption, "ESP32 restart, tunggu sekitar 3 detik. Musik dan koneksi berhenti.");
    lv_obj_set_style_text_color(caption, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(caption, FONT_SMALL, 0);
    lv_obj_set_style_pad_top(caption, 6, 0);

}
