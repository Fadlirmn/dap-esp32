// Volume settings -- on-screen -/+ control for what was previously only
// reachable by physically turning the rotary knob (see input.cpp). Reuses
// overlay_volume.cpp's existing clamp/HUD/i2s-volume logic entirely; this
// screen is just another caller of overlay_volume_adjust(), same as the
// knob's ISR.
#include "screen_volume.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"
#include "overlay_volume.h"

static lv_obj_t *s_pct_lbl = NULL;
static lv_obj_t *s_bar = NULL;

static void refresh(void) {
    int v = overlay_volume_get();
    if (s_pct_lbl) lv_label_set_text_fmt(s_pct_lbl, "%d%%", v);
    if (s_bar) lv_bar_set_value(s_bar, v, LV_ANIM_ON);
}

static void dec_cb(lv_event_t *e) { LV_UNUSED(e); overlay_volume_adjust(-5); refresh(); }
static void inc_cb(lv_event_t *e) { LV_UNUSED(e); overlay_volume_adjust(+5); refresh(); }

static void on_screen_deleted(lv_event_t *e) {
    LV_UNUSED(e);
    s_pct_lbl = NULL;
    s_bar = NULL;
}

void screen_volume_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    ui_header_create(scr, LV_SYMBOL_VOLUME_MAX, "Volume", true);
    lv_obj_add_event_cb(scr, on_screen_deleted, LV_EVENT_DELETE, NULL);

    s_pct_lbl = lv_label_create(scr);
    lv_label_set_text_fmt(s_pct_lbl, "%d%%", overlay_volume_get());
    lv_obj_set_style_text_color(s_pct_lbl, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(s_pct_lbl, FONT_TITLE, 0);
    lv_obj_align(s_pct_lbl, LV_ALIGN_TOP_MID, 0, 40);

    s_bar = lv_bar_create(scr);
    lv_obj_set_size(s_bar, SCREEN_W - 80, 8);
    lv_obj_align(s_bar, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_radius(s_bar, 4, 0);
    lv_obj_set_style_bg_color(s_bar, COLOR_VOLUME_TRACK, 0);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_bar, 4, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_bar, COLOR_SUCCESS, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, overlay_volume_get(), LV_ANIM_OFF);

    lv_obj_t *minus = ui_button_create(scr, SCREEN_W / 2 - 80, 110, 56, 44, RADIUS_BUTTON_CIRCLE, COLOR_SURFACE_BUTTON);
    lv_obj_center(icon_symbol(minus, LV_SYMBOL_MINUS, COLOR_TEXT_PRIMARY, FONT_TITLE));
    lv_obj_add_event_cb(minus, dec_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *plus = ui_button_create(scr, SCREEN_W / 2 + 24, 110, 56, 44, RADIUS_BUTTON_CIRCLE, COLOR_SURFACE_BUTTON);
    lv_obj_center(icon_symbol(plus, LV_SYMBOL_PLUS, COLOR_TEXT_PRIMARY, FONT_TITLE));
    lv_obj_add_event_cb(plus, inc_cb, LV_EVENT_CLICKED, NULL);

    ui_footer_create(scr, "+/- ubah volume (langkah 5) - MENU: kembali");
}
