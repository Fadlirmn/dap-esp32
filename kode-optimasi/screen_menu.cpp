// L3 "Menu mode" — 2-column mode-select grid.
#include "screen_menu.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"
#include "screen_manager.h"
#include <stdint.h>

static void tile_clicked_cb(lv_event_t *e) {
    ScreenId target = (ScreenId)(intptr_t)lv_event_get_user_data(e);
    screen_manager_show(target);
}

static void make_tile(lv_obj_t *scr, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                       bool selected, const char *symbol, const char *label, const char *badge,
                       ScreenId target) {
    lv_obj_t *tile = ui_button_create(scr, x, y, w, h, RADIUS_ROW, selected ? COLOR_SELECTED : COLOR_SURFACE);
    lv_obj_add_event_cb(tile, tile_clicked_cb, LV_EVENT_CLICKED, (void *)(intptr_t)target);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(tile, 6, 0);
    lv_obj_set_style_pad_column(tile, 8, 0);

    lv_obj_t *icon_badge = lv_obj_create(tile);
    lv_obj_remove_style_all(icon_badge);
    lv_obj_set_size(icon_badge, 28, 28);
    lv_obj_set_style_radius(icon_badge, RADIUS_ICON_BADGE, 0);
    lv_obj_set_style_bg_color(icon_badge, COLOR_SURFACE_BUTTON, 0);
    lv_obj_set_style_bg_opa(icon_badge, LV_OPA_COVER, 0);
    lv_obj_center(icon_symbol(icon_badge, symbol, COLOR_ACCENT, FONT_SMALL));

    lv_obj_t *lbl = lv_label_create(tile);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl, FONT_BODY, 0);

    if (badge) {
        lv_obj_t *b = lv_label_create(tile);
        lv_label_set_text(b, badge);
        lv_obj_set_style_text_color(b, COLOR_SUCCESS, 0);
        lv_obj_set_style_text_font(b, FONT_SMALL, 0);
    }
}

void screen_menu_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    ui_header_create(scr, NULL, "Menu", false);

    const lv_coord_t w = (SCREEN_W - 16 - 6) / 2, h = 40;
    const lv_coord_t x0 = 8, x1 = x0 + w + 6;
    const lv_coord_t y0 = 30, row_step = h + 6;

    make_tile(scr, x0, y0 + 0 * row_step, w, h, false, LV_SYMBOL_LIST, "Daftar Lagu", NULL, SCREEN_LIBRARY);
    make_tile(scr, x1, y0 + 0 * row_step, w, h, false, LV_SYMBOL_AUDIO, "Playlist", NULL, SCREEN_PLAYLISTS);
    make_tile(scr, x0, y0 + 1 * row_step, w, h, false, LV_SYMBOL_PLAY, "Local Player", NULL, SCREEN_NOW_PLAYING);
    make_tile(scr, x1, y0 + 1 * row_step, w, h, false, LV_SYMBOL_BLUETOOTH, "BT Receiver", NULL, SCREEN_BT_DEVICES_RX);
    make_tile(scr, x0, y0 + 2 * row_step, w, h, false, LV_SYMBOL_BLUETOOTH, "BT Transmitter", NULL, SCREEN_BT_DEVICES_TX);
    make_tile(scr, x1, y0 + 2 * row_step, w, h, false, LV_SYMBOL_UPLOAD, "Transfer File", NULL, SCREEN_TRANSFER);
    make_tile(scr, x0, y0 + 3 * row_step, w, h, false, LV_SYMBOL_LEFT, "Kembali", NULL, SCREEN_NOW_PLAYING);
    make_tile(scr, x1, y0 + 3 * row_step, w, h, false, LV_SYMBOL_SETTINGS, "Equalizer", NULL, SCREEN_EQUALIZER);

    ui_footer_create(scr, "Klik: OK - MENU: kembali");
}
