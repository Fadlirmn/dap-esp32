#pragma once
// "Pilih Perangkat" -- device list shown before BT Receiver/Transmitter.
// Not one of the original 12 dap-ui-landscape boards; added so there's an
// actual menu to connect to a device instead of Receiver/Transmitter just
// claiming "Terhubung" with nothing chosen.
#include "lvgl.h"

lv_obj_t *screen_bt_devices_rx_create(lv_obj_t *parent); // picking a device to receive FROM (phone)
lv_obj_t *screen_bt_devices_tx_create(lv_obj_t *parent); // picking a device to send TO (speaker)
