// No-op stand-in for the ESP32 Arduino core's WebServer.h: no real HTTP
// server on desktop, so handleClient() never actually dispatches anything.
// Just enough shape for wifi_transfer.cpp to compile and link unmodified.
#pragma once
#include "Arduino.h"
#include <cstdint>

enum HTTPMethod { HTTP_GET, HTTP_POST };

enum HTTPUploadStatus { UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END, UPLOAD_FILE_ABORTED };

struct HTTPUpload {
    HTTPUploadStatus status = UPLOAD_FILE_START;
    String filename;
    uint8_t *buf = nullptr;
    size_t currentSize = 0;
    size_t totalSize = 0;
};

typedef void (*THandlerFunction)();

class WebServer {
public:
    WebServer(int = 80) {}
    void on(const char *, HTTPMethod, THandlerFunction) {}
    void on(const char *, HTTPMethod, THandlerFunction, THandlerFunction) {}
    void begin() {}
    void handleClient() {}
    void send(int, const char * = "", const char * = "") {}
    void send_P(int, const char *, const char *) {}
    HTTPUpload &upload() { static HTTPUpload u; return u; }
};
