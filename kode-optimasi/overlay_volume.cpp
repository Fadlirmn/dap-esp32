#include "overlay_volume.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"

static lv_obj_t *s_pill = NULL;
static lv_timer_t *s_hide_timer = NULL;
static int s_volume = 70; // persistent real state -- survives across show() calls, not re-derived from a timer

static void hide_timer_cb(lv_timer_t *t) {
    if (s_pill) { lv_obj_del(s_pill); s_pill = NULL; }
    lv_timer_del(t);
    s_hide_timer = NULL;
}

void overlay_volume_show(int percent) {
    if (s_pill) { lv_obj_del(s_pill); s_pill = NULL; }
    if (s_hide_timer) { lv_timer_del(s_hide_timer); s_hide_timer = NULL; }

    bool muted = percent <= 0;
    lv_color_t accent = muted ? COLOR_ERROR : COLOR_SUCCESS;

    s_pill = ui_card_create(lv_layer_top(), 14, 34, SCREEN_W - 28, 44, RADIUS_PILL, COLOR_SURFACE_BUTTON);
    lv_obj_set_style_shadow_width(s_pill, 10, 0);
    lv_obj_set_style_shadow_color(s_pill, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(s_pill, LV_OPA_60, 0);
    lv_obj_set_flex_flow(s_pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_pill, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(s_pill, 10, 0);
    lv_obj_set_style_pad_column(s_pill, 8, 0);

    icon_symbol(s_pill, muted ? LV_SYMBOL_MUTE : LV_SYMBOL_VOLUME_MAX, COLOR_TEXT_MUTED, FONT_BODY);

    lv_obj_t *track = lv_bar_create(s_pill);
    lv_obj_set_flex_grow(track, 1);
    lv_obj_set_height(track, 6);
    lv_obj_set_style_radius(track, 3, 0);
    lv_obj_set_style_radius(track, 3, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(track, COLOR_VOLUME_TRACK, 0);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(track, accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_bar_set_range(track, 0, 100);
    lv_bar_set_value(track, percent, LV_ANIM_OFF);

    lv_obj_t *val = lv_label_create(s_pill);
    lv_label_set_text_fmt(val, "%d", percent);
    lv_obj_set_style_text_color(val, accent, 0);
    lv_obj_set_style_text_font(val, FONT_BODY, 0);

    s_hide_timer = lv_timer_create(hide_timer_cb, 2000, NULL);
    lv_timer_set_repeat_count(s_hide_timer, 1);
}

void overlay_volume_adjust(int delta) {
    s_volume += delta;
    if (s_volume < 0) s_volume = 0;
    if (s_volume > 100) s_volume = 100;
    overlay_volume_show(s_volume);
}
