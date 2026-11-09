// WiFi AP + HTTP upload server, writes received files onto the SD card.
// Ported verbatim from final-form/wifi_transfer.cpp/.h (no UI coupling there
// either) -- kode-optimasi has no boot-time mode gate, so the AP just runs
// for the whole boot instead of only in a dedicated MODE_TRANSFER_FILE.
#pragma once
#include <Arduino.h>

extern String wifiTransfer_ssid;
extern String wifiTransfer_ip;
extern volatile int wifiTransfer_lastUploadPct;        // -1 = no upload in progress, 0-100 while receiving
extern volatile unsigned long wifiTransfer_filesReceived;

void wifiTransferSetup();  // starts the AP + HTTP server; call once in setup()
void wifiTransferPoll();   // call every loop() tick (server.handleClient())
