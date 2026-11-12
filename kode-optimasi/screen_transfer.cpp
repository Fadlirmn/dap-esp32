// L6 "Transfer File" — WiFi-AP file transfer status screen.
#include "screen_transfer.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"
#include <stdio.h>
#ifndef KODE_OPTIMASI_SIM
#include "wifi_transfer.h"
#endif

#ifndef KODE_OPTIMASI_SIM
static lv_obj_t *s_files_lbl = NULL;
static lv_obj_t *s_status_lbl = NULL;

static void on_screen_deleted(lv_event_t *e) {
    LV_UNUSED(e);
    s_files_lbl = s_status_lbl = NULL;
}
#endif

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
    ui_header_create(scr, LV_SYMBOL_UPLOAD, "Transfer File", true);

    lv_obj_t *hotspot = info_card(scr, 12, 32, 150, 52);
    card_line(hotspot, "Hotspot ESP32", COLOR_TEXT_MUTED, FONT_SMALL);
#ifdef KODE_OPTIMASI_SIM
    card_line(hotspot, "DAP-ESP32", COLOR_TEXT_PRIMARY, FONT_BODY);
#else
    card_line(hotspot, wifiTransfer_ssid.c_str(), COLOR_TEXT_PRIMARY, FONT_BODY);
#endif
    card_line(hotspot, "Password: dap12345", COLOR_TEXT_MUTED, FONT_SMALL);

    lv_obj_t *url = info_card(scr, 12, 92, 150, 52);
    card_line(url, "Buka di browser", COLOR_TEXT_MUTED, FONT_SMALL);
#ifdef KODE_OPTIMASI_SIM
    card_line(url, "http://192.168.4.1", COLOR_ACCENT, FONT_BODY);
#else
    String url_text = "http://" + wifiTransfer_ip;
    card_line(url, url_text.c_str(), COLOR_ACCENT, FONT_BODY);
#endif
    card_line(url, "atau http://dap.local", COLOR_TEXT_MUTED, FONT_SMALL);

    // Right column mirrors the left: two equal-height list cards, same style.
    lv_obj_t *xfer = info_card(scr, 170, 32, SCREEN_W - 182, 52);
    card_line(xfer, "File diterima", COLOR_TEXT_MUTED, FONT_SMALL);
#ifdef KODE_OPTIMASI_SIM
    card_line(xfer, "Aurora.mp3", COLOR_TEXT_PRIMARY, FONT_BODY);
    card_line(xfer, "2140 KB", COLOR_TEXT_MUTED, FONT_SMALL);
#else
    s_files_lbl = card_line(xfer, "0 file", COLOR_TEXT_PRIMARY, FONT_BODY);
#endif

    lv_obj_t *status = info_card(scr, 170, 92, SCREEN_W - 182, 52);
    card_line(status, "Status", COLOR_TEXT_MUTED, FONT_SMALL);
#ifdef KODE_OPTIMASI_SIM
    lv_obj_t *status_val = card_line(status, "BT dan putar mati", COLOR_TEXT_PRIMARY, FONT_SMALL);
#else
    s_status_lbl = card_line(status, "Menunggu upload...", COLOR_TEXT_PRIMARY, FONT_SMALL);
    lv_obj_t *status_val = s_status_lbl;
    lv_obj_add_event_cb(status, on_screen_deleted, LV_EVENT_DELETE, NULL);
#endif
    lv_label_set_long_mode(status_val, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(status_val, SCREEN_W - 182 - 16); // card width minus its own padding, so wrap instead of clipping

    ui_footer_create(scr, "MENU: menu / keluar");
}

void screen_transfer_tick(void) {
#ifndef KODE_OPTIMASI_SIM
    if (s_files_lbl) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%lu file", wifiTransfer_filesReceived);
        lv_label_set_text(s_files_lbl, buf);
    }
    if (s_status_lbl) {
        int pct = wifiTransfer_lastUploadPct;
        if (pct < 0) lv_label_set_text(s_status_lbl, "Menunggu upload...");
        else if (pct < 100) lv_label_set_text_fmt(s_status_lbl, "Mengunggah... %d%%", pct);
        else lv_label_set_text(s_status_lbl, "Selesai");
    }
#endif
}
