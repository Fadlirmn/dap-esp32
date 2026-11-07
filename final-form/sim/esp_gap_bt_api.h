// Minimal stand-in for the ESP-IDF BT GAP header bluetooth.h includes directly.
#pragma once
#include <cstdint>

typedef uint8_t esp_bd_addr_t[6];
#define ESP_BD_ADDR_LEN 6
typedef int esp_err_t;
#define ESP_OK 0

typedef enum {
    ESP_BT_INQ_MODE_GENERAL_INQUIRY = 0,
} esp_bt_inq_mode_t;

#define ESP_BT_COD_SRVC_RENDERING 0x04
#define ESP_BT_COD_SRVC_AUDIO 0x08
#define ESP_BT_COD_SRVC_TELEPHONY 0x20

inline esp_err_t esp_bt_gap_start_discovery(esp_bt_inq_mode_t, int, int) { return ESP_OK; }
