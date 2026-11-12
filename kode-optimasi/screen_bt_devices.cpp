// "Pilih Perangkat" -- shared screen *name* for both BT Receiver and
// Transmitter, but the real-hardware behavior genuinely differs by role (see
// bt_backend.h): a sink (Receiver) is passive -- the phone connects to it,
// there's nothing to pick from a list, so no Scan button -- while a source
// (Transmitter) actively scans, so it gets a real "Scan" button + live
// result list. The sim/ preview below mirrors that split with a fake 10s-free
// scan (Scan reveals a canned dummy list) and a status-only RX, since there's
// no real BT radio to drive here.
#include "screen_bt_devices.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"
#include "screen_manager.h"
#include <stdint.h>
#ifndef KODE_OPTIMASI_SIM
#include "bt_backend.h"
#endif

#ifdef KODE_OPTIMASI_SIM
struct BtDevice { const char *name; };
// Speakers nearby that this ESP32 (as the source) can stream out to --
// revealed by pressing Scan, same two-step flow as the real scan below.
static const BtDevice kTxDevices[] = {
    {"JBL Flip 6"},
    {"Sony SRS-XB13"},
    {"Speaker Mobil"},
};
#define TX_COUNT (int)(sizeof(kTxDevices) / sizeof(kTxDevices[0]))

static bool s_tx_scanned = false; // sim-only stand-in for bt_tx_scan_active()/_count()

static void tx_row_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    screen_manager_show(SCREEN_TRANSMITTER);
}

static void tx_scan_btn_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    s_tx_scanned = true;
    screen_manager_show(SCREEN_BT_DEVICES_TX); // rebuild with the now-revealed list
}

lv_obj_t *screen_bt_devices_rx_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    ui_header_create(scr, LV_SYMBOL_BLUETOOTH, "Pilih Perangkat", true);

    lv_obj_t *status = lv_label_create(scr);
    lv_label_set_text(status, "Menunggu ponsel terhubung...");
    lv_obj_set_style_text_color(status, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(status, FONT_BODY, 0);
    lv_obj_center(status);

    ui_footer_create(scr, "Sink BT pasif - ponsel yang menghubungkan - MENU kembali");
    return scr;
}

lv_obj_t *screen_bt_devices_tx_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    ui_header_create(scr, LV_SYMBOL_BLUETOOTH, "Pilih Perangkat", true);

    lv_obj_t *scan_btn = ui_button_create(scr, SCREEN_W - 86, 30, 70, 26, RADIUS_BUTTON_LG, COLOR_ACCENT);
    lv_obj_t *scan_lbl = lv_label_create(scan_btn);
    lv_label_set_text(scan_lbl, "Scan");
    lv_obj_set_style_text_color(scan_lbl, COLOR_ACCENT_TEXT, 0);
    lv_obj_set_style_text_font(scan_lbl, FONT_SMALL, 0);
    lv_obj_center(scan_lbl);
    lv_obj_add_event_cb(scan_btn, tx_scan_btn_clicked_cb, LV_EVENT_CLICKED, NULL);

    if (!s_tx_scanned) {
        lv_obj_t *hint = lv_label_create(scr);
        lv_label_set_text(hint, "Tekan Scan untuk mencari speaker");
        lv_obj_set_style_text_color(hint, COLOR_TEXT_MUTED, 0);
        lv_obj_set_style_text_font(hint, FONT_SMALL, 0);
        lv_obj_set_pos(hint, 12, 36);
        ui_footer_create(scr, "MENU: kembali");
        return scr;
    }

    const lv_coord_t row_h = 32, row_gap = 4, first_y = 58;
    for (int i = 0; i < TX_COUNT; i++) {
        lv_obj_t *row = ui_list_row_create(scr, first_y + i * (row_h + row_gap), row_h, false);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, tx_row_clicked_cb, LV_EVENT_CLICKED, NULL);
        lv_group_add_obj(screen_manager_group(), row);
        icon_symbol(row, LV_SYMBOL_BLUETOOTH, COLOR_ACCENT, FONT_SMALL);
        ui_row_text_create(row, kTxDevices[i].name, "");
    }

    ui_footer_create(scr, "Klik untuk hubungkan - MENU kembali");
    return scr;
}

#else // real hardware: RX = status-only, TX = real scan

static void rx_forget_btn_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    bt_rx_forget_device();
    screen_manager_show(SCREEN_BT_DEVICES_RX);
}

lv_obj_t *screen_bt_devices_rx_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    bt_backend_start_receiver(); // idempotent -- becomes the sink if not already
    ui_header_create(scr, LV_SYMBOL_BLUETOOTH, "Pilih Perangkat", true);

    lv_obj_t *status = lv_label_create(scr);
    bool connected = bt_rx_connected();
    lv_label_set_text(status, connected ? bt_rx_device_name() : "Menunggu ponsel terhubung...");
    lv_obj_set_style_text_color(status, connected ? COLOR_TEXT_PRIMARY : COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(status, FONT_BODY, 0);
    lv_obj_center(status);

    if (connected) {
        lv_obj_t *forget_btn = ui_button_create(scr, (SCREEN_W - 120) / 2, 150, 120, 32,
                                                 RADIUS_BUTTON_LG, COLOR_SURFACE_BUTTON);
        lv_obj_t *lbl = lv_label_create(forget_btn);
        lv_label_set_text(lbl, "Lupakan");
        lv_obj_set_style_text_color(lbl, COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(forget_btn, rx_forget_btn_clicked_cb, LV_EVENT_CLICKED, NULL);
    }

    ui_footer_create(scr, "Sink BT pasif - ponsel yang menghubungkan - MENU kembali");
    return scr;
}

// --- TX: real classic-BT inquiry scan (manual trigger, 10s window) ---
static lv_obj_t *s_tx_list = NULL;
static lv_obj_t *s_tx_status_lbl = NULL;
static lv_timer_t *s_tx_timer = NULL;

static void tx_result_row_clicked_cb(lv_event_t *e) {
    uint8_t idx = (uint8_t)(intptr_t)lv_event_get_user_data(e);
    bt_tx_connect_scan_result(idx);
    screen_manager_show(SCREEN_TRANSMITTER);
}

static void rebuild_tx_rows(void) {
    if (!s_tx_list) return;
    lv_obj_clean(s_tx_list);
    uint8_t n = bt_tx_scan_count();
    if (bt_tx_scan_active()) {
        lv_label_set_text_fmt(s_tx_status_lbl, "Mencari... (%u ditemukan)", n);
    } else if (n == 0) {
        lv_label_set_text(s_tx_status_lbl, "Tidak ada speaker ditemukan - tekan Scan");
    } else {
        lv_label_set_text_fmt(s_tx_status_lbl, "%u speaker ditemukan", n);
    }

    const lv_coord_t row_h = 32, row_gap = 4;
    for (uint8_t i = 0; i < n; i++) {
        lv_obj_t *row = ui_list_row_create(s_tx_list, i * (row_h + row_gap), row_h, false);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, tx_result_row_clicked_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_group_add_obj(screen_manager_group(), row);
        icon_symbol(row, LV_SYMBOL_BLUETOOTH, COLOR_ACCENT, FONT_SMALL);
        ui_row_text_create(row, bt_tx_scan_name(i), "");
    }
}

static void tx_poll_timer_cb(lv_timer_t *t) {
    LV_UNUSED(t);
    rebuild_tx_rows();
    if (!bt_tx_scan_active() && s_tx_timer) {
        lv_timer_del(s_tx_timer);
        s_tx_timer = NULL;
    }
}

static void tx_scan_btn_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    bt_tx_start_scan();
    if (!s_tx_timer) s_tx_timer = lv_timer_create(tx_poll_timer_cb, 400, NULL);
    rebuild_tx_rows();
}

static void on_tx_screen_deleted(lv_event_t *e) {
    LV_UNUSED(e);
    if (s_tx_timer) {
        lv_timer_del(s_tx_timer);
        s_tx_timer = NULL;
    }
    s_tx_list = NULL;
    s_tx_status_lbl = NULL;
}

lv_obj_t *screen_bt_devices_tx_create(lv_obj_t *parent) {
    lv_obj_t *scr = parent;
    bt_backend_start_transmitter(); // idempotent -- becomes the source if not already
    ui_header_create(scr, LV_SYMBOL_BLUETOOTH, "Pilih Perangkat", true);

    lv_obj_t *scan_btn = ui_button_create(scr, SCREEN_W - 86, 30, 70, 26, RADIUS_BUTTON_LG, COLOR_ACCENT);
    lv_obj_t *scan_lbl = lv_label_create(scan_btn);
    lv_label_set_text(scan_lbl, "Scan");
    lv_obj_set_style_text_color(scan_lbl, COLOR_ACCENT_TEXT, 0);
    lv_obj_set_style_text_font(scan_lbl, FONT_SMALL, 0);
    lv_obj_center(scan_lbl);
    lv_obj_add_event_cb(scan_btn, tx_scan_btn_clicked_cb, LV_EVENT_CLICKED, NULL);

    s_tx_status_lbl = lv_label_create(scr);
    lv_obj_set_pos(s_tx_status_lbl, 12, 36);
    lv_obj_set_style_text_color(s_tx_status_lbl, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(s_tx_status_lbl, FONT_SMALL, 0);

    s_tx_list = lv_obj_create(scr);
    lv_obj_remove_style_all(s_tx_list);
    lv_obj_set_pos(s_tx_list, 0, 58);
    lv_obj_set_size(s_tx_list, SCREEN_W, SCREEN_H - 58 - 22);
    lv_obj_add_event_cb(s_tx_list, on_tx_screen_deleted, LV_EVENT_DELETE, NULL);

    if (bt_tx_scan_active() && !s_tx_timer) s_tx_timer = lv_timer_create(tx_poll_timer_cb, 400, NULL);
    rebuild_tx_rows();

    ui_footer_create(scr, "Klik untuk hubungkan - MENU kembali");
    return scr;
}

#endif // KODE_OPTIMASI_SIM
