// Band editor for whichever preset is currently active (equalizerGetPreset())
// -- every preset is editable now, not just Custom, per the user's request
// to be able to tweak Flat/Bass Boost/etc. too and reset them back. 3 band
// rows, each a "-"/"+" stepper (1dB/click, +-12dB) instead of a slider --
// same reasoning as screen_equalizer.cpp, the physical rotary knob is
// hard-wired to volume (see input.cpp), so PREV/NEXT tabbing + ENC_SW click
// is the only input path; a stepper fits that, a drag slider wouldn't.
// Changes apply live; "Simpan" is a separate explicit step so a quick
// experiment doesn't wear out the SD card every click, and "Reset" discards
// edits back to the preset's factory gains.
#include "screen_equalizer_custom.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "display_pins.h"
#include "screen_manager.h"
#include "equalizer.h"
#include <stdio.h>

static const char *kBandNames[3] = {"Bass", "Mid", "Treble"};
static lv_obj_t *s_value_lbl[3] = {NULL, NULL, NULL};
static lv_obj_t *s_save_lbl = NULL; // the Save button's own label -- doubles as its status text
static lv_timer_t *s_save_revert_timer = NULL;

static void format_db(char *buf, size_t n, float db) {
    snprintf(buf, n, "%+.0f dB", db);
}

static void refresh_value_labels(void) {
    float g[3];
    equalizerGetGains(equalizerGetPreset(), g);
    for (int band = 0; band < 3; band++) {
        if (!s_value_lbl[band]) continue;
        char buf[12];
        format_db(buf, sizeof(buf), g[band]);
        lv_label_set_text(s_value_lbl[band], buf);
    }
}

// user_data packs band (bits 1..) and direction (bit 0: 1 = +, 0 = -).
static void adjust_band_cb(lv_event_t *e) {
    intptr_t packed = (intptr_t)lv_event_get_user_data(e);
    int band = (int)(packed >> 1);
    int dir = (packed & 1) ? 1 : -1;

    EqPreset p = equalizerGetPreset();
    float g[3];
    equalizerGetGains(p, g);
    g[band] += dir;
    equalizerSetGains(p, g[0], g[1], g[2]);
    refresh_value_labels(); // re-read post-clamp
}

static void reset_btn_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    equalizerResetGains(equalizerGetPreset());
    refresh_value_labels();
}

static void revert_save_label_cb(lv_timer_t *t) {
    if (s_save_lbl) {
        lv_label_set_text(s_save_lbl, "Simpan ke SD");
        lv_obj_set_style_text_color(s_save_lbl, COLOR_ACCENT_TEXT, 0);
    }
    lv_timer_del(t);
    s_save_revert_timer = NULL;
}

static void save_btn_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    bool ok = equalizerSaveToSD();
    if (s_save_lbl) {
        lv_label_set_text(s_save_lbl, ok ? "Tersimpan!" : "Gagal (cek SD)");
        lv_obj_set_style_text_color(s_save_lbl, ok ? COLOR_SUCCESS : COLOR_ERROR, 0);
    }
    if (s_save_revert_timer) lv_timer_del(s_save_revert_timer);
    s_save_revert_timer = lv_timer_create(revert_save_label_cb, 1500, NULL);
    lv_timer_set_repeat_count(s_save_revert_timer, 1);
}

static void on_screen_deleted(lv_event_t *e) {
    LV_UNUSED(e);
    for (int i = 0; i < 3; i++) s_value_lbl[i] = NULL;
    if (s_save_revert_timer) { lv_timer_del(s_save_revert_timer); s_save_revert_timer = NULL; }
    s_save_lbl = NULL;
}

static lv_obj_t *make_step_btn(lv_obj_t *row, const char *symbol, intptr_t user_data) {
    lv_obj_t *btn = ui_button_create(row, 0, 0, 30, 30, RADIUS_CHIP, COLOR_SURFACE_BUTTON);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, symbol);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl, FONT_BODY, 0);
    lv_obj_center(lbl);
    lv_obj_add_event_cb(btn, adjust_band_cb, LV_EVENT_CLICKED, (void *)user_data);
    return btn;
}

void screen_equalizer_custom_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    // Caller (screen_equalizer.cpp) already called equalizerSetPreset() for
    // whichever row was clicked -- this screen just edits "the active one".
    EqPreset preset = equalizerGetPreset();
    ui_header_create(scr, LV_SYMBOL_SETTINGS, equalizerPresetName(preset), true);
    lv_obj_add_event_cb(scr, on_screen_deleted, LV_EVENT_DELETE, NULL);

    float gains[3];
    equalizerGetGains(preset, gains);

    const lv_coord_t row_h = 40, row_gap = 6, first_y = 32;
    for (int band = 0; band < 3; band++) {
        lv_obj_t *row = ui_list_row_create(scr, first_y + band * (row_h + row_gap), row_h, false);

        lv_obj_t *name = lv_label_create(row);
        lv_label_set_text(name, kBandNames[band]);
        lv_obj_set_style_text_color(name, COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(name, FONT_BODY, 0);
        lv_obj_set_width(name, 56);

        lv_obj_t *value = lv_label_create(row);
        char buf[12];
        format_db(buf, sizeof(buf), gains[band]);
        lv_label_set_text(value, buf);
        lv_obj_set_style_text_color(value, COLOR_ACCENT, 0);
        lv_obj_set_style_text_font(value, FONT_BODY, 0);
        lv_obj_set_flex_grow(value, 1);
        lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, 0);
        s_value_lbl[band] = value;

        make_step_btn(row, LV_SYMBOL_MINUS, (intptr_t)(band << 1) | 0);
        make_step_btn(row, LV_SYMBOL_PLUS, (intptr_t)(band << 1) | 1);
    }

    const lv_coord_t btn_y = first_y + 3 * (row_h + row_gap) + 6, btn_w = 104, btn_gap = 10;
    const lv_coord_t btns_x0 = (SCREEN_W - (2 * btn_w + btn_gap)) / 2;

    lv_obj_t *reset_btn = ui_button_create(scr, btns_x0, btn_y, btn_w, 32, RADIUS_BUTTON_LG, COLOR_SURFACE_BUTTON);
    lv_obj_t *reset_lbl = lv_label_create(reset_btn);
    lv_label_set_text(reset_lbl, "Reset");
    lv_obj_set_style_text_color(reset_lbl, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(reset_lbl, FONT_SMALL, 0);
    lv_obj_center(reset_lbl);
    lv_obj_add_event_cb(reset_btn, reset_btn_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *save_btn = ui_button_create(scr, btns_x0 + btn_w + btn_gap, btn_y, btn_w, 32,
                                           RADIUS_BUTTON_LG, COLOR_ACCENT);
    s_save_lbl = lv_label_create(save_btn);
    lv_label_set_text(s_save_lbl, "Simpan");
    lv_obj_set_style_text_color(s_save_lbl, COLOR_ACCENT_TEXT, 0);
    lv_obj_set_style_text_font(s_save_lbl, FONT_SMALL, 0);
    lv_obj_center(s_save_lbl);
    lv_obj_add_event_cb(save_btn, save_btn_clicked_cb, LV_EVENT_CLICKED, NULL);

    ui_footer_create(scr, "+/- ubah band - Reset ke bawaan - Simpan agar tahan restart");
}
