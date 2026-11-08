// L6 "Transfer File" — WiFi-AP file transfer status screen.
#include "screen_transfer.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"

static lv_obj_t *info_card(lv_obj_t *scr, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h) {
    lv_obj_t *card = ui_card_create(scr, x, y, w, h, 16, COLOR_SURFACE);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

static lv_obj_t *card_line(lv_obj_t *card, const char *text, lv_color_t color, const lv_font_t *font) {
    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, font, 0);
    return lbl;
}

void screen_transfer_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    ui_header_create(scr, LV_SYMBOL_UPLOAD, "Transfer File", true, 78);

    lv_obj_t *hotspot = info_card(scr, 12, 32, 150, 52);
    card_line(hotspot, "Hotspot ESP32", COLOR_TEXT_MUTED, FONT_SMALL);
    card_line(hotspot, "DAP-ESP32", COLOR_TEXT_PRIMARY, FONT_BODY);
    card_line(hotspot, "Password: 12345678", COLOR_TEXT_MUTED, FONT_SMALL);

    lv_obj_t *url = info_card(scr, 12, 92, 150, 52);
    card_line(url, "Buka di browser", COLOR_TEXT_MUTED, FONT_SMALL);
    card_line(url, "http://192.168.4.1", COLOR_ACCENT, FONT_BODY);
    card_line(url, "atau http://dap.local", COLOR_TEXT_MUTED, FONT_SMALL);

    // Right column mirrors the left: two equal-height list cards, same style,
    // ending at the same y as the hotspot/url cards (no more "Menerima..."
    // status line, no special pill widget -- just plain list cards).
    lv_obj_t *xfer = info_card(scr, 170, 32, SCREEN_W - 182, 52);
    card_line(xfer, "File diterima", COLOR_TEXT_MUTED, FONT_SMALL);
#ifdef KODE_OPTIMASI_SIM
    card_line(xfer, "Aurora.mp3", COLOR_TEXT_PRIMARY, FONT_BODY);
    card_line(xfer, "2140 KB", COLOR_TEXT_MUTED, FONT_SMALL);
#else
    // No file actually in flight in the real build -- a future pass wires
    // this to the real WiFi-upload handler's current transfer, if any.
    card_line(xfer, "Menunggu file...", COLOR_TEXT_PRIMARY, FONT_BODY);
#endif

    lv_obj_t *status = info_card(scr, 170, 92, SCREEN_W - 182, 52);
    card_line(status, "Status", COLOR_TEXT_MUTED, FONT_SMALL);
    lv_obj_t *status_val = card_line(status, "BT dan putar mati", COLOR_TEXT_PRIMARY, FONT_SMALL);
    lv_label_set_long_mode(status_val, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(status_val, SCREEN_W - 182 - 16); // card width minus its own padding, so wrap instead of clipping

    ui_footer_create(scr, "MENU: menu / keluar");
}
