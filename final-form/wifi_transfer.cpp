// WiFi AP + HTTP upload server, writes received files onto the SD card.
// For MODE_TRANSFER_FILE only.
#include "wifi_transfer.h"
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>

String wifiTransfer_ssid;
String wifiTransfer_ip;
volatile int wifiTransfer_lastUploadPct = -1;
volatile unsigned long wifiTransfer_filesReceived = 0;

static const char *AP_SSID = "DAP-Transfer";
static const char *AP_PASSWORD = "dap12345";  // >=8 chars, required for WPA2

static WebServer server(80);
static File uploadFile;
static bool uploadOk = false;  // false => destination dir missing / SD not ready, skip writes

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>DAP File Transfer</title>
<style>body{font-family:sans-serif;margin:2em;}input,button{font-size:1.1em;margin:0.5em 0;}</style>
</head><body>
<h3>DAP File Transfer</h3>
<p>Upload .mp3 files (go to /tracks) or .m3u playlists (go to /Playlists).</p>
<form method="POST" action="/upload" enctype="multipart/form-data">
<input type="file" name="upload" multiple><br>
<button type="submit">Upload</button>
</form>
</body></html>
)HTML";

static void handleRoot() {
    wifiTransfer_lastUploadPct = -1;  // fresh page load => no upload in flight
    server.send_P(200, "text/html", INDEX_HTML);
}

static void handleUploadDone() {
    server.send(200, "text/plain", "OK");
}

static void handleFileUpload() {
    HTTPUpload &up = server.upload();

    if (up.status == UPLOAD_FILE_START) {
        wifiTransfer_lastUploadPct = 0;
        String name = up.filename;
        int slash = name.lastIndexOf('/');
        if (slash >= 0) name = name.substring(slash + 1);

        String destDir = name.endsWith(".m3u") || name.endsWith(".M3U") ? "/Playlists" : "/tracks";
        uploadOk = SD.exists(destDir) || SD.mkdir(destDir);
        if (uploadOk) {
            uploadFile = SD.open(destDir + "/" + name, FILE_WRITE);
            uploadOk = (bool)uploadFile;
        }
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (uploadOk && uploadFile) uploadFile.write(up.buf, up.currentSize);
        // No reliable total size from chunked multipart; coarse progress only.
        wifiTransfer_lastUploadPct = 50;
    } else if (up.status == UPLOAD_FILE_END) {
        if (uploadOk && uploadFile) {
            uploadFile.close();
            wifiTransfer_filesReceived++;
        }
        wifiTransfer_lastUploadPct = 100;
    } else if (up.status == UPLOAD_FILE_ABORTED) {
        if (uploadFile) uploadFile.close();
        wifiTransfer_lastUploadPct = -1;
    }
}

void wifiTransferSetup() {
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    wifiTransfer_ssid = AP_SSID;
    wifiTransfer_ip = WiFi.softAPIP().toString();
    wifiTransfer_lastUploadPct = -1;
    wifiTransfer_filesReceived = 0;

    server.on("/", HTTP_GET, handleRoot);
    server.on("/upload", HTTP_POST, handleUploadDone, handleFileUpload);
    server.begin();
}

void wifiTransferPoll() {
    server.handleClient();
}
