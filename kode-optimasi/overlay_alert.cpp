#include "overlay_alert.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"

static lv_obj_t *s_battery_backdrop = NULL;
static lv_obj_t *s_toast = NULL;
static lv_timer_t *s_toast_timer = NULL;

static void dismiss_battery_dialog(lv_event_t *e) {
    LV_UNUSED(e);
    if (s_battery_backdrop) { lv_obj_del(s_battery_backdrop); s_battery_backdrop = NULL; }
}

void overlay_alert_show_low_battery(int percent) {
    if (s_battery_backdrop) lv_obj_del(s_battery_backdrop);

    s_battery_backdrop = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_battery_backdrop);
    lv_obj_set_size(s_battery_backdrop, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(s_battery_backdrop, COLOR_OVERLAY_DIM, 0);
    lv_obj_set_style_bg_opa(s_battery_backdrop, LV_OPA_70, 0);
    lv_obj_clear_flag(s_battery_backdrop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_battery_backdrop, LV_OBJ_FLAG_CLICKABLE); // blocks clicks reaching the screen below (modal)

    lv_obj_t *dialog = ui_card_create(s_battery_backdrop, 30, 28, SCREEN_W - 60, LV_SIZE_CONTENT,
                                       RADIUS_DIALOG, COLOR_SURFACE_BUTTON);
    lv_obj_set_style_pad_all(dialog, 16, 0);
    lv_obj_set_flex_flow(dialog, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_row = lv_obj_create(dialog);
    lv_obj_remove_style_all(title_row);
    lv_obj_set_size(title_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(title_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(title_row, 8, 0);
    icon_symbol(title_row, LV_SYMBOL_BATTERY_EMPTY, COLOR_ERROR, FONT_BODY);
    lv_obj_t *title = lv_label_create(title_row);
    lv_label_set_text(title, "Baterai rendah");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, FONT_BODY, 0);

    lv_obj_t *body = lv_label_create(dialog);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(body, SCREEN_W - 92);
    lv_label_set_text_fmt(body, "Tinggal %d%%. Colok USB ke D32 untuk mengisi daya.", percent);
    lv_obj_set_style_text_color(body, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(body, FONT_SMALL, 0);
    lv_obj_set_style_pad_top(body, 6, 0);

    lv_obj_t *ok_row = lv_obj_create(dialog);
    lv_obj_remove_style_all(ok_row);
    lv_obj_set_size(ok_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(ok_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ok_row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(ok_row, 10, 0);

    lv_obj_t *ok_btn = lv_label_create(ok_row);
    lv_label_set_text(ok_btn, "OK");
    lv_obj_set_style_text_color(ok_btn, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(ok_btn, FONT_BODY, 0);
    lv_obj_add_flag(ok_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ok_btn, dismiss_battery_dialog, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_text_opa(ok_btn, LV_OPA_60, LV_STATE_PRESSED); // press feedback, see ui_common.cpp's ui_button_create
}

static void toast_timer_cb(lv_timer_t *t) {
    if (s_toast) { lv_obj_del(s_toast); s_toast = NULL; }
    lv_timer_del(t);
    s_toast_timer = NULL;
}

void overlay_alert_show_sd_error(void) {
    if (s_toast) lv_obj_del(s_toast);
    if (s_toast_timer) { lv_timer_del(s_toast_timer); s_toast_timer = NULL; }

    s_toast = ui_card_create(lv_layer_top(), 12, SCREEN_H - 14 - 36, SCREEN_W - 24, LV_SIZE_CONTENT,
                              RADIUS_TOAST, COLOR_TOAST_BG);
    lv_obj_set_style_pad_all(s_toast, 12, 0);
    lv_obj_clear_flag(s_toast, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(s_toast);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, SCREEN_W - 48);
    lv_label_set_text(lbl, "SD card tidak terbaca. Cek kartu dan kabel.");
    lv_obj_set_style_text_color(lbl, COLOR_TOAST_TEXT, 0);
    lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);

    s_toast_timer = lv_timer_create(toast_timer_cb, 3000, NULL);
    lv_timer_set_repeat_count(s_toast_timer, 1);
}
