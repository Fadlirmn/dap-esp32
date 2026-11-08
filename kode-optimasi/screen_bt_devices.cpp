// "Pilih Perangkat" -- shared device-list screen for both BT Receiver (pick
// a phone to receive from) and BT Transmitter (pick a speaker to send to).
// Same layout/logic, just a different demo list + different target screen.
#include "screen_bt_devices.h"
#include "ui_common.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "display_pins.h"
#include "screen_manager.h"
#include <stdint.h>

struct BtDevice { const char *name; };

#ifdef KODE_OPTIMASI_SIM
// RX: phones that have paired with this ESP32 before (it's the one being
// streamed to, so "devices" here means "previously-paired senders").
static const BtDevice kRxDevices[] = {
    {"Pixel 7 - Fadli"},
    {"iPhone - Sarah"},
    {"Laptop Kerja"},
};
// TX: speakers nearby that this ESP32 (as the source) can stream out to.
static const BtDevice kTxDevices[] = {
    {"JBL Flip 6"},
    {"Sony SRS-XB13"},
    {"Speaker Mobil"},
};
#define RX_COUNT (int)(sizeof(kRxDevices) / sizeof(kRxDevices[0]))
#define TX_COUNT (int)(sizeof(kTxDevices) / sizeof(kTxDevices[0]))
#else
// No real BT scan/pairing in the real build -- a future pass wires this to
// the actual A2DP device list; empty-state handled below. (RX_COUNT/TX_COUNT
// as plain 0 -- not a sizeof-on-array trick -- since there's no array here.)
static const BtDevice *kRxDevices = NULL;
static const BtDevice *kTxDevices = NULL;
#define RX_COUNT 0
#define TX_COUNT 0
#endif

static void rx_row_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    screen_manager_show(SCREEN_RECEIVER);
}

static void tx_row_clicked_cb(lv_event_t *e) {
    LV_UNUSED(e);
    screen_manager_show(SCREEN_TRANSMITTER);
}

static lv_obj_t *create(lv_obj_t *scr, const char *title, const BtDevice *devices, int count,
                         lv_event_cb_t row_clicked_cb, const char *empty_msg) {
    ui_header_create(scr, LV_SYMBOL_BLUETOOTH, title, true, 78);

    if (count == 0) {
        lv_obj_t *empty = lv_label_create(scr);
        lv_label_set_text(empty, empty_msg);
        lv_obj_set_style_text_color(empty, COLOR_TEXT_MUTED, 0);
        lv_obj_set_style_text_font(empty, FONT_BODY, 0);
        lv_obj_center(empty);
        ui_footer_create(scr, "MENU: kembali");
        return scr;
    }

    const lv_coord_t row_h = 32, row_gap = 4, first_y = 30;
    for (int i = 0; i < count; i++) {
        lv_obj_t *row = ui_list_row_create(scr, first_y + i * (row_h + row_gap), row_h, false);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, row_clicked_cb, LV_EVENT_CLICKED, NULL);
        icon_symbol(row, LV_SYMBOL_BLUETOOTH, COLOR_ACCENT, FONT_SMALL);
        ui_row_text_create(row, devices[i].name, "");
    }

    ui_footer_create(scr, "Klik untuk hubungkan - MENU kembali");
    return scr;
}

lv_obj_t *screen_bt_devices_rx_create(lv_obj_t *parent) {
    return create(parent, "Pilih Perangkat", kRxDevices, RX_COUNT, rx_row_clicked_cb,
                   "Tidak ada perangkat yang pernah terhubung");
}

lv_obj_t *screen_bt_devices_tx_create(lv_obj_t *parent) {
    return create(parent, "Pilih Perangkat", kTxDevices, TX_COUNT, tx_row_clicked_cb,
                   "Tidak ada speaker ditemukan");
}
