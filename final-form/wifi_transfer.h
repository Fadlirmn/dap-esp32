// WiFi AP + HTTP upload server, writes received files onto the SD card.
// For MODE_TRANSFER_FILE only.
#pragma once
#include <Arduino.h>

extern String wifiTransfer_ssid;
extern String wifiTransfer_ip;
extern volatile int wifiTransfer_lastUploadPct;        // -1 = no upload in progress, 0-100 while receiving
extern volatile unsigned long wifiTransfer_filesReceived;

void wifiTransferSetup();  // starts the AP + HTTP server; call once in setup() when operatingMode == MODE_TRANSFER_FILE
void wifiTransferPoll();   // call every loop() tick (server.handleClient())
