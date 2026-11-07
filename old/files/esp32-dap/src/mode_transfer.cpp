// Mode 4: Transfer File lewat WiFi (web upload ke /Music). BT & playback tidak aktif di mode ini.
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SD.h>
#include "modes.h"
#include "common.h"
#include "ui.h"

static WebServer server(80);
static File upFile;
static String upName, ipText, ssidText;
static uint32_t upBytes = 0;
static uint16_t upCount = 0;
static bool sdOk = false, isAP = false;

static const char PAGE_HEAD[] PROGMEM = R"rawliteral(<!doctype html><html><head><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1"><title>DAP ESP32</title>
<style>body{font:16px system-ui;background:#0b0f14;color:#e6eaf0;max-width:640px;margin:auto;padding:16px}
a{color:#38bdf8}li{margin:8px 0;display:flex;justify-content:space-between;gap:12px}
button,input{font:inherit;margin:4px 0}progress{width:100%}small{color:#8a94a0}</style></head><body>
<h2>DAP ESP32</h2>
<input type=file id=f multiple accept="audio/*"><br><button onclick=up()>Upload</button>
<progress id=p value=0 max=100 hidden></progress><p id=s></p>
<h3>/Music</h3><ul>)rawliteral";

static const char PAGE_TAIL[] PROGMEM = R"rawliteral(</ul><script>
async function up(){const fs=document.getElementById('f').files,p=document.getElementById('p'),s=document.getElementById('s');
p.hidden=false;for(let i=0;i<fs.length;i++){await new Promise(r=>{const x=new XMLHttpRequest(),d=new FormData();
d.append('f',fs[i]);x.upload.onprogress=e=>{p.value=e.loaded/e.total*100;s.textContent=(i+1)+'/'+fs.length+'  '+fs[i].name};
x.onloadend=r;x.open('POST','/upload');x.send(d)})}location.reload()}
</script></body></html>)rawliteral";

static String safeName(const String &in) {
  String n = in.substring(max(in.lastIndexOf('/'), in.lastIndexOf('\\')) + 1);
  String out;
  for (size_t i = 0; i < n.length(); i++) {
    char c = n[i];
    bool ok = isalnum((unsigned char)c) || c == '.' || c == '-' || c == '_' || c == ' ' || c == '(' || c == ')';
    out += ok ? c : '_';
  }
  out.trim();
  if (out.startsWith(".")) out = "_" + out;
  return out;
}

static String urlEnc(const String &s) {
  String o;
  char b[4];
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (isalnum((unsigned char)c) || c == '.' || c == '-' || c == '_') o += c;
    else { snprintf(b, sizeof(b), "%%%02X", (unsigned char)c); o += b; }
  }
  return o;
}

static void handleRoot() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent_P(PAGE_HEAD);
  File dir = SD.open(MUSIC_DIR);
  if (dir && dir.isDirectory()) {
    File f = dir.openNextFile();
    while (f) {
      if (!f.isDirectory()) {
        String n = f.name();
        String li = "<li><span>" + n + " <small>" + String(f.size() / 1024) + " KB</small></span>" +
                    "<a href='/del?f=" + urlEnc(n) + "'>hapus</a></li>";
        server.sendContent(li);
      }
      f.close();
      f = dir.openNextFile();
    }
    dir.close();
  }
  server.sendContent_P(PAGE_TAIL);
  server.sendContent("");
}

static void handleUpload() {
  HTTPUpload &u = server.upload();
  if (u.status == UPLOAD_FILE_START) {
    if (!SD.exists(MUSIC_DIR)) SD.mkdir(MUSIC_DIR);
    upName = safeName(u.filename);
    String p = String(MUSIC_DIR) + "/" + upName;
    if (SD.exists(p)) SD.remove(p);
    upFile = SD.open(p, FILE_WRITE);
    upBytes = 0;
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (upFile) { upFile.write(u.buf, u.currentSize); upBytes += u.currentSize; }
  } else if (u.status == UPLOAD_FILE_END) {
    if (upFile) upFile.close();
    upName = ""; upCount++;
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    if (upFile) { upFile.close(); SD.remove(String(MUSIC_DIR) + "/" + upName); }
    upName = "";
  }
}

static void handleDelete() {
  String n = safeName(server.arg("f"));
  if (n.length()) SD.remove(String(MUSIC_DIR) + "/" + n);
  server.sendHeader("Location", "/");
  server.send(303);
}

static void drawInfo() {
  uiHeader("Transfer File");
  tft.fillRect(0, 26, 320, 188, C_BG);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString(isAP ? "Hotspot ESP32" : "WiFi", 16, 36, 2);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString(ssidText, 16, 54, 4);
  if (isAP) {
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString(String("Password: ") + WIFI_AP_PASS, 16, 84, 2);
  }
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("Buka di browser:", 16, 112, 2);
  tft.setTextColor(C_ACCENT, C_BG);
  tft.drawString("http://" + ipText, 16, 130, 4);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString(String("atau http://dap.local   SD: ") + String((uint32_t)(SD.cardSize() / (1024ULL * 1024ULL * 1024ULL))) + " GB", 16, 164, 2);
  uiFooter("MENU: menu / keluar");
}

void transferSetup() {
  uiHeader("Transfer File");
  sdOk = storageBegin();
  if (!sdOk) { uiMessage("SD card tidak terbaca", nullptr, C_BAD); return; }
  if (!SD.exists(MUSIC_DIR)) SD.mkdir(MUSIC_DIR);

  uiMessage("Menghubungkan WiFi...");
  bool sta = false;
  if (strlen(WIFI_STA_SSID)) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) delay(200);
    sta = (WiFi.status() == WL_CONNECTED);
  }
  if (sta) {
    isAP = false; ssidText = WIFI_STA_SSID; ipText = WiFi.localIP().toString();
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    isAP = true; ssidText = WIFI_AP_SSID; ipText = WiFi.softAPIP().toString();
  }
  WiFi.setSleep(false);
  MDNS.begin("dap");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/upload", HTTP_POST, []() { server.send(200, "text/plain", "OK"); }, handleUpload);
  server.on("/del", HTTP_GET, handleDelete);
  server.begin();
  drawInfo();
}

void transferLoop() {
  BtnEvent b = buttonPoll();
  NavEvent nv = navPoll(false);
  if (!sdOk) {
    if (b == BTN_LONG || b == BTN_CLICK) { uiMenu(false, nullptr); uiHeader("Transfer File"); uiMessage("SD card tidak terbaca", nullptr, C_BAD); }
    delay(10);
    return;
  }
  server.handleClient();

  if (b == BTN_LONG || nv == NAV_MENU) {
    uiMenu(false, []() { server.handleClient(); });
    drawInfo();
  }

  static uint32_t drawAt = 0;
  static String lastStatus;
  uint32_t now = millis();
  if (now - drawAt > 400) {
    drawAt = now;
    String st = upName.length() ? ("Menerima: " + upName + "  " + String(upBytes / 1024) + " KB")
                                : (upCount ? "Selesai: " + String(upCount) + " file" : String("Siap menerima file"));
    if (st != lastStatus) {
      lastStatus = st;
      tft.fillRect(0, 186, 320, 24, C_BG);
      tft.setTextColor(upName.length() ? C_WARN : C_GOOD, C_BG);
      String s = st.length() > 34 ? st.substring(0, 32) + ".." : st;
      tft.drawString(s, 16, 190, 2);
    }
  }
}
