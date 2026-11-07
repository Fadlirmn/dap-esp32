// No-op stand-in for the ESP32 Arduino core's WiFi.h: no real radio on desktop.
#pragma once
#include "Arduino.h"

class IPAddress {
public:
    String toString() const { return String("192.168.4.1"); }
};

class WiFiClass {
public:
    bool softAP(const char *, const char * = nullptr) { return true; }
    IPAddress softAPIP() { return IPAddress(); }
};
inline WiFiClass WiFi;
