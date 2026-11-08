# PRD — kode-optimasi: LVGL UI untuk Digital Audio Player (ESP32)

| | |
|---|---|
| **Versi dokumen** | 1.5 |
| **Tanggal** | 2026-11-08 |
| **Status** | Reverse-engineered dari kode berjalan (bukan draft rencana) |
| **Lingkup kode yang didokumentasikan** | `/mnt/seagate/Koding/DAP-Esp32Devkit/kode-optimasi` — 40 file, ~1.950 baris |
| **Riwayat revisi** | v1.0 (2026-11-08): draf awal. v1.1 (2026-11-08): + ID requirement & acceptance criteria per layar (§4), + diagram navigasi (§6), + metadata versi ini. v1.2 (2026-11-08): + detail lengkap efek transisi/perpindahan menu, efek hover/pressed, interaksi overlay-vs-transisi (§5.6–§5.9); ditemukan & dicatat gap baru: FR-L8 tidak reachable dari UI mana pun. v1.3 (2026-11-08): banyak elemen yang dulu dekoratif sekarang benar-benar nyata (play/pause, prev/next per-track duration, volume persisten + scroll wheel, pilih baris Daftar Lagu/Playlist, badge MENU jadi ikon home, efek tekan universal, jam berhenti saat pause); 2 bug visual nyata ditemukan & diperbaiki (karakter "·" tidak didukung font bawaan, elemen klik tanpa feedback visual sama sekali). v1.4 (2026-11-08): + layar baru **Pilih Perangkat** (`screen_bt_devices.cpp`, FR-NEW-1) di luar 12 board asli — Menu's tile BT Receiver/Transmitter sekarang lewat sini dulu, badge "Terhubung" bisa diklik balik untuk ganti perangkat; data dummy semua layar (termasuk ini) sudah dipisah agar hanya ada di build sim (`KODE_OPTIMASI_SIM`), dikonfirmasi lewat `strings` di ELF ESP32 nyata, bukan cuma baca kode. v1.5 (2026-11-08): **Bluetooth klasik jadi nyata di hardware** (`bt_backend.h/.cpp`, §3.8, di-port dari `final-form/bluetooth.*`+`bt_receiver.*`) — TX scan manual-trigger 10s + connect + auto-reconnect tersimpan (`Preferences`); RX status koneksi pasif + AVRCP passthrough nyata (play/pause/prev/next ke ponsel terhubung); FR-NEW-1/FR-L4/FR-L5 diupdate untuk bedakan perilaku sim vs hardware (sebelumnya disamakan). Overflow partisi flash default ditemukan & diperbaiki (`huge_app.csv`) — diverifikasi lewat `pio run -e esp32dev` sukses (RAM 26.5%/86.7KB, Flash 52.8%/1.66MB dari 3MB), bukan klaim. |

> **Catatan**: Dokumen ini adalah PRD yang *di-reverse-engineer* dari source code yang sudah ada di `/mnt/seagate/Koding/DAP-Esp32Devkit/kode-optimasi`, bukan PRD untuk fitur yang belum dibuat. Setiap angka, warna, dan perilaku di sini diambil langsung dari kode (per baris dicek ulang saat menulis dokumen ini), bukan dari ingatan/asumsi percakapan.

---

## 1. Ringkasan Eksekutif

**Problem Statement**: Firmware DAP yang nyata (`../final-form/`) memakai Adafruit_GFX mentah (bukan LVGL) dan baru mengimplementasikan 1 dari 12 layar desain (`Now Playing`). Tidak ada implementasi LVGL sama sekali di repo ini sebelumnya, dan tidak ada cara memverifikasi tampilan 12 layar penuh tanpa flash ke hardware.

**Solusi**: Port LVGL 8.x standalone dari seluruh 12 layar mockup `dap-ui-landscape/` + `DAP_UI_landscape_240x320_pixel_export/` ke modul baru (`kode-optimasi/`) yang:
- Dioptimasi untuk ESP32 WROOM-32 **tanpa PSRAM** (partial draw buffer, memory pool kecil, lazy screen lifecycle).
- Memisahkan task FreeRTOS (GUI di core 1, slot kosong untuk audio di core 0).
- Punya animasi reel kaset berbasis vektor (bukan bitmap/GIF).
- Bisa di-build & di-uji tanpa hardware lewat preview desktop SDL (`sim/`).

**Kriteria Sukses (terukur, sudah diverifikasi)**:
1. Build ESP32 (`pio run`, `esp32dev`/Arduino) sukses dengan `lvgl@8.4.0` + `TFT_eSPI@2.5.43` — **RAM 17.8% (58.252 B / 327.680 B), Flash 62.8% (822.900 B / 1.310.720 B)**.
2. `static_assert` memastikan draw buffer partial (320×15×2 byte = 9.600 byte) tidak pernah melebihi budget 16 KB saat compile.
3. Semua 12 board mockup punya representasi 1:1 di kode (lihat tabel §4).
4. Preview desktop (`sim/`) jalan ≥60 detik tanpa crash, dan navigasi klik nyata (MENU badge → Menu → tile → layar tujuan → Playlists → detail → kembali) terverifikasi berfungsi lewat mouse indev SDL.
5. Tidak ada aset gambar mentah/GIF untuk animasi apa pun — seluruh gerakan dibangun dari widget vektor LVGL (`lv_line`, style-transform) atau glyph font bawaan (`LV_SYMBOL_*`).

---

## 2. Lingkup & Non-Goals

**Di dalam lingkup**:
- 12 layar/overlay LVGL dengan data demo statis (teks sama seperti di mockup, mis. "Aurora 3:42").
- Tema warna Material You Dark sesuai 5 nilai hex yang diminta eksplisit (lihat §3.4).
- Animasi reel kaset vektor, transisi antar-layar hemat RAM, overlay volume & alert.
- Pemisahan task FreeRTOS (GUI vs slot audio).
- Preview desktop SDL (`sim/`) sebagai alat verifikasi tanpa hardware.
- Navigasi klik nyata untuk sebagian jalur (MENU badge, tile Menu, baris Playlists, back-chevron Playlist Detail).

**Non-Goals (sengaja tidak dikerjakan di modul ini)**:
- **Tidak ada backend audio/SD/WiFi nyata**: tidak membaca SD card, tidak decode MP3/I2S, tidak WiFi AP sungguhan. Semua angka durasi lagu/SSID/IP/% baterai adalah teks statis demo.
- **Bluetooth klasik (scan/pairing/koneksi) SEKARANG NYATA di hardware** (`bt_backend.*`, v1.5) — lihat §3.8 dan FR-NEW-1/FR-L4/FR-L5 untuk detail; satu-satunya bagian yang masih stub adalah payload audio-nya sendiri (sumber A2DP mengirim silence, karena belum ada decoder di modul ini).
- **Tidak ada input fisik di hardware asli**: `kode-optimasi.ino` tidak mendaftarkan `lv_indev` apa pun (tidak ada touch XPT2046, tidak ada tombol). Klik hanya berfungsi di `sim/` (mouse SDL).
- **Tidak mengubah `final-form/` atau `dap-ui-landscape/`** — modul berdiri sendiri, tidak ada `#include` lintas-folder.
- **Tidak semua tombol transport punya handler klik** — lihat §6 untuk daftar pasti mana yang sudah nyata vs sekadar dekoratif.
- Font "bold" asli (LVGL bawaan tidak punya varian bold; dipakai regular 18px sebagai pendekatan).

---

## 3. Arsitektur Sistem

### 3.1 Peta Modul (36 file, ~1.550 baris)

| Kategori | File |
|---|---|
| App shell | `kode-optimasi.ino`, `lv_conf.h`, `display_pins.h`, `TFT_eSPI_User_Setup.h`, `platformio.ini` |
| Tema & shared UI | `ui_theme.h`, `ui_common.h/.cpp`, `ui_icons.h/.cpp` |
| Orkestrasi layar | `screen_manager.h/.cpp`, `demo_cycle.h/.cpp` |
| 8 layar penuh | `screen_now_playing.*`, `screen_library.*`, `screen_menu.*`, `screen_receiver.*`, `screen_transmitter.*`, `screen_transfer.*`, `screen_playlists.*`, `screen_playlist_detail.*` |
| 1 layar transisi | `screen_switch.*` |
| 2 overlay top-layer | `overlay_volume.*`, `overlay_alert.*` |
| Animasi khusus | `cassette_anim.h/.cpp` |
| Preview desktop | `sim/sim_main.cpp`, `sim/Makefile` |

### 3.2 Siklus hidup layar (lazy, hemat RAM)

- **Satu root screen persisten** dibuat sekali (`screen_manager_init` → `ui_screen_create()` → `lv_scr_load()`), **tidak pernah di-load ulang** setelah itu.
- `screen_manager_show(id)`:
  1. `lv_obj_clean(s_root)` — hapus **semua** child layar lama dulu (termasuk semua `lv_anim_t` yang menempel padanya, otomatis oleh LVGL).
  2. Baru kemudian `build_screen(s_root, id)` membangun layar baru.
- Dampak: RAM puncak selalu "satu layar penuh", bukan dua. Ini **bukan desain awal** — desain awal memakai `lv_scr_load_anim(..., auto_del=true)` yang membangun layar baru **sebelum** menghapus yang lama, sehingga sesaat dua layar penuh ada di memori. Di `sim/` dengan `LV_MEM_SIZE` sengaja dikecilkan (24 KB), ini **segfault nyata** (`lv_theme_apply` kehabisan memori) — ditemukan lewat pengujian, bukan review statis, lalu diperbaiki ke desain `lv_obj_clean` saat ini.

### 3.3 Task FreeRTOS (`kode-optimasi.ino`)

| Task | Core | Priority | Stack | Isi |
|---|---|---|---|---|
| `guiTask` | 1 | 2 | 10240 | `lv_tick_inc()` + `lv_timer_handler()` tiap **66 ms** (±15 FPS), satu-satunya task yang menyentuh API LVGL → **tidak perlu mutex** di seluruh kodebase. |
| `audioTaskStub` | 0 | 3 | 4096 | Kerangka kosong (`vTaskDelay(1000ms)` + komentar `TODO`) — tempat pipeline I2S nyata nanti, sesuai permintaan asli "pisahkan task Core 0/Core 1". |
| Arduino `loop()` | 1 (default) | — | — | Hanya `vTaskDelay(1000ms)` — semua kerja nyata ada di 2 task di atas. |

### 3.4 Buffer gambar & memori (budget no-PSRAM)

- **Partial draw buffer**: `320 × 15 baris × 2 byte (RGB565) = 9.600 byte (~9,4 KB)`, array statis (bukan `malloc`), dicek dengan `static_assert(sizeof(s_draw_buf1) < 16*1024, ...)` di `kode-optimasi.ino` — gagal compile kalau suatu saat berubah dan melebihi budget.
- **LVGL memory pool** (`LV_MEM_SIZE` di `lv_conf.h`): **24 KB** — dipilih cukup untuk satu layar widget sekaligus (sesuai desain lazy lifecycle §3.2).
- **Hasil nyata di ESP32** (`pio run`, diverifikasi): RAM 58.252 byte / 327.680 byte (17,8%), Flash 822.900 byte / 1.310.720 byte (62,8%).

### 3.5 Sistem Tema (`ui_theme.h`)

5 token di bawah **sengaja menimpa** nilai asli mockup, sesuai spesifikasi Material You Dark yang diminta eksplisit di awal proyek:

| Token | Nilai dipakai | Nilai asli mockup | Dipakai untuk |
|---|---|---|---|
| `COLOR_BG` | `#121619` | `#101418` | Background semua layar |
| `COLOR_CARD_BLUE` | `#0F3870` | `#00468C` | Kartu hero Now Playing + chip "Favorit" |
| `COLOR_ACCENT` | `#A4C8FF` | `#A8C8FF` | Indikator progress, tombol play, teks aktif |
| `COLOR_TRACK` | `#3B4C5E` | `#33383F` | Track progress bar (bagian belum terisi) |
| `COLOR_TEXT_MUTED` | `#8C9BAE` | `#C3C6CF` / `#A3ACB8` | Subtitle, meta text, footer |

Token lain (tidak ada di 5 nilai spek, diambil langsung dari mockup): `COLOR_SURFACE #1C2026`, `COLOR_SURFACE_BUTTON #272A30`, `COLOR_SELECTED #3A4658`, `COLOR_ACCENT_TEXT #003062`, `COLOR_TEXT_PRIMARY #E2E2E9`, `COLOR_SUCCESS #7DDB9A`, `COLOR_SUCCESS_DARK #00391F`, `COLOR_ERROR #FFB4AB`, `COLOR_WARNING #FFD97D`, `COLOR_BT_HERO #3B2A5F`, `COLOR_BT_HERO_ICON #E9DDFF`, `COLOR_ON_ACCENT_DIM #D6E3FF`, `COLOR_CASSETTE_WINDOW #001B3E`, `COLOR_SCROLLBAR #8B9199`, `COLOR_TOAST_BG #E2E2E9`, `COLOR_TOAST_TEXT #2E3036`, `COLOR_OVERLAY_DIM #0B0E12`, `COLOR_VOLUME_TRACK #4A5059`, + 4 warna chip playlist (`#5A3A1A` coklat, `#1F4A3A` hijau, plus reuse `COLOR_SELECTED`/`COLOR_CARD_BLUE`/`COLOR_BT_HERO`).

Radius: `RADIUS_CARD=24` (sesuai spek awal "radius 24px"), `RADIUS_BUTTON_LG=18`, `RADIUS_BUTTON_CIRCLE=21`, `RADIUS_BADGE=10`, `RADIUS_ROW=12`, `RADIUS_CHIP=8`, `RADIUS_ICON_BADGE=14`, `RADIUS_DIALOG=28`, `RADIUS_TOAST=8`, `RADIUS_PILL=22`.

Font: `FONT_TITLE=montserrat_18`, `FONT_BODY=montserrat_14`, `FONT_SMALL=montserrat_12` — semua bawaan LVGL, tidak ada font custom.

### 3.6 Sistem Ikon (`ui_icons.h/.cpp`)

Prinsip: **tidak ada bitmap/raster sama sekali**. Dua sumber ikon:
1. **Glyph bawaan LVGL** (`LV_SYMBOL_*`) — dipakai langsung sebagai `lv_label` lewat helper `icon_symbol()`. Mencakup: `PLAY, PAUSE, PREV, NEXT, SHUFFLE, LOOP, BLUETOOTH, OK, MUTE, VOLUME_MAX, UPLOAD, LIST, BARS, LEFT, AUDIO, WIFI`, dan 5 level `BATTERY_FULL/3/2/1/EMPTY`.
2. **3 widget komposit custom** (karena LVGL tidak punya glyph untuk ini):
   - `icon_battery(parent, percent)` — pilih glyph baterai otomatis sesuai persen + label `"NN%"`, warna jadi `COLOR_ERROR` di ≤15%.
   - `icon_connected_badge(parent)` — lingkaran hijau 22×22 + centang putih (badge "Terhubung" di layar BT).
   - `icon_equalizer_bars(parent, color)` — 3 bar vertikal statis tinggi berbeda (6/12/9 px) sebagai marker "sedang diputar" di baris daftar lagu/playlist (pengganti nomor indeks).

### 3.7 Chrome bersama (`ui_common.h/.cpp`)

- `ui_screen_create()` — root layar penuh (320×240, `COLOR_BG`).
- `ui_header_create(scr, icon, title, show_menu_badge, battery%)` — header 26px: ikon+judul kiri, badge "MENU" (klik → §6) + baterai kanan.
- `ui_footer_create(scr, hint)` — footer 28px, teks hint abu-abu.
- `ui_card_create` / `ui_button_create` — kotak rounded generik; button otomatis `LV_OBJ_FLAG_CLICKABLE`.
- `ui_list_row_create` + `ui_row_text_create` — baris list standar (dipakai Daftar Lagu & Daftar Playlist).

### 3.8 Backend Bluetooth klasik nyata (`bt_backend.h/.cpp`) — baru di v1.5

Di-port dari `final-form/bluetooth.*` (A2DP source: scan/connect) dan
`final-form/bt_receiver.*` (A2DP sink: koneksi + AVRCP passthrough), pakai
library [ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP) yang sama,
dilepas dari UI coupling karena `screen_manager.cpp` sudah punya navigasinya
sendiri. **Tidak ikut di-link ke `sim/`** (desktop SDL tidak punya radio BT
nyata) — setiap titik panggilnya di `screen_*.cpp` sudah di-gate
`#ifndef KODE_OPTIMASI_SIM`, pola yang sama dengan gating data dummy.

- **Satu peran aktif pada satu waktu** (source XOR sink), sama seperti
  `final-form/`. Masuk ke layar BT terkait memulai peran itu sesuai kebutuhan
  (idempotent), bukan dipilih saat boot seperti `final-form/` — karena di sini
  user ganti-ganti dari Menu saat berjalan.
- **Transmitter (source) — scan nyata, dipicu manual**: masuk "Pilih
  Perangkat" untuk TX **tidak** auto-scan (classic-BT inquiry lambat & boros
  daya) — tombol "Scan" menjalankan inquiry 10 detik (`bt_tx_start_scan()`),
  hasil muncul live (di-poll setiap 400ms selama aktif), tap hasil →
  connect + simpan (`Preferences`, selamat dari reboot) untuk auto-reconnect.
- **Receiver (sink) — pasif, tidak ada scan/pick-list**: sink tidak bisa
  "scan" ponsel — ponsel yang menghubungi dia — jadi "Pilih Perangkat" untuk
  RX cuma status koneksi (nama perangkat terhubung, atau "Menunggu ponsel
  terhubung...") + tombol "Lupakan" untuk putus pairing saat ini.
- **AVRCP passthrough nyata**: tombol play/pause/prev/next di BT Receiver
  mengirim command transport sungguhan ke ponsel yang terhubung
  (`bt_rx_play_pause()` dkk.) — tidak perlu decoder lokal karena ini cuma
  sinyal remote-control, sama seperti tombol headset BT.
- **Masih stub**: sumber A2DP mengirim silence (`tx_fill_silence` di
  `bt_backend.cpp`, belum ada SD/decoder di modul ini) dan tombol transport
  BT Transmitter sendiri (tidak ada sumber lokal untuk dikontrol, tetap
  dekoratif seperti sebelumnya).
- **Verifikasi nyata**: build `pio run -e esp32dev` dengan backend ini
  ter-link sukses — tapi overflow partisi default ESP32 (stack classic-BT
  sendirian sudah besar), diperbaiki sama seperti `final-form/`:
  `board_build.partitions = huge_app.csv` di `platformio.ini`. Hasil:
  **RAM 26.5% (86.7 KB), Flash 52.8% (1.66 MB dari 3 MB)**.

---

## 4. Requirement Fungsional per Layar (12 board asli + 1 tambahan)

| # | Board mockup | File | Jenis |
|---|---|---|---|
| *(baru)* | Pilih Perangkat (RX/TX) | `screen_bt_devices.*` | Layar penuh, dipakai 2x (lihat FR-NEW-1) — **di luar 12 board asli** |
| L1 | Now Playing | `screen_now_playing.*` + `cassette_anim.*` | Layar penuh |
| L2 | Daftar Lagu | `screen_library.*` | Layar penuh |
| L3 | Menu mode | `screen_menu.*` | Layar penuh |
| L4 | BT Receiver | `screen_receiver.*` | Layar penuh |
| L5 | BT Transmitter | `screen_transmitter.*` | Layar penuh |
| L6 | Transfer File | `screen_transfer.*` | Layar penuh |
| L7 | Baterai rendah + SD error | `overlay_alert.*` | 2 popup top-layer (modal + toast) |
| L8 | Beralih mode | `screen_switch.*` | Layar transisi tanpa chrome |
| L9/L10 | Volume berubah / senyap | `overlay_volume.*` | 1 widget top-layer, 2 state warna |
| L11 | Daftar playlist | `screen_playlists.*` | Layar penuh |
| L12 | Isi playlist | `screen_playlist_detail.*` | Layar penuh |

Legenda status verifikasi yang dipakai di tiap FR di bawah:
✅ Diimplementasikan & tampil benar di `sim/` (visual dicek manual) — ⚙️ Diimplementasikan, berfungsi secara struktural, belum ada verifikasi visual pixel-exact — 🖱️ Interaktif (punya klik-handler nyata) — 🪫 Elemen dekoratif (terlihat seperti kontrol tapi tanpa handler, lihat §6).

#### FR-L1 — Now Playing (Local Player)
**File**: `screen_now_playing.cpp` + `cassette_anim.cpp` | **Status**: ✅ render, 🖱️ transport nyata
- [x] Header: judul "Local Player", badge MENU (🖱️ → Menu, ikon home), baterai 78% (dummy, disengaja — lihat §8).
- [x] Kartu hero biru `COLOR_CARD_BLUE` radius 24px, 296×92px, berisi animasi reel kaset (§5.1).
- [x] Ikon shuffle + loop di pojok kanan-atas kartu (dekoratif, tanpa engine shuffle/repeat nyata).
- [x] Strip judul + subtitle mengikuti track yang sedang dipilih (state persisten `s_track_idx`, bukan teks statis).
- [x] Progress bar + label waktu, di-tick tiap 1 detik (§5.2) — **durasi total mengikuti lagu yang aktif** (`kDemoTracks[].duration_sec`, bukan satu durasi tetap untuk semua lagu), dan **berhenti bertambah saat pause** (`now_playing_is_playing()` dicek sebelum tick).
- [x] Tombol prev/next (🖱️): benar-benar mengganti track (5 lagu demo), reset elapsed ke 0, posisi persisten antar rebuild layar.
- [x] Tombol play/pause (🖱️): toggle nyata — ganti ikon PLAY/PAUSE, pause/resume animasi reel (§5.1), dan menghentikan/melanjutkan tick waktu.
- [x] Footer: "PREV/NEXT pilih lagu - Klik putar/pause".
- [x] API publik: `now_playing_set_track_info()`, `now_playing_set_progress()`, `now_playing_set_playing()`, `now_playing_is_playing()`, `now_playing_tick_elapsed()`, `now_playing_set_battery()` (stub no-op).

#### FR-L2 — Daftar Lagu
**File**: `screen_library.cpp` | **Status**: ✅ render, 🖱️ baris interaktif
- [x] Header ikon `LIST` + "Daftar Lagu" + badge MENU + baterai.
- [x] 5 baris demo tinggi 32px: Aurora 3:42, Blue Hour 4:05, Dreamcatcher 5:12, Echoes 2:58, City Lights 3:50 — baris "sedang diputar" (marker equalizer bar, bukan nomor indeks) **berpindah lewat klik baris** (`s_now_playing_idx` persisten, via `screen_manager_show(SCREEN_LIBRARY)` dipanggil ulang dari dalam handler klik baris itu sendiri — pola yang sama dipakai di seluruh app, terbukti aman di `sim/`).
- [x] Scrollbar dekoratif statis 3×36px di tepi kanan (bukan scrollbar fungsional — baris tetap tidak bisa discroll sungguhan).
- [x] Footer: "3 / 42 - PREV/NEXT pilih - Klik putar".
- Catatan: hanya memindah marker "sedang diputar"; tidak benar-benar memutar lagu (tidak ada backend audio).

#### FR-L3 — Menu Mode
**File**: `screen_menu.cpp` | **Status**: ✅ render, 🖱️ navigasi penuh — satu-satunya layar dengan semua elemen interaktif
- [x] Header "Menu" saja, **tanpa** badge MENU (karena sudah berada di Menu).
- [x] Grid 2 kolom × 4 baris, 7 tile: Daftar Lagu, Playlist, Local Player (+badge hijau "AKTIF"), **BT Receiver (tile fokus/highlight visual)**, BT Transmitter, Transfer File, Kembali.
- [x] Tiap tile: badge ikon 28×28 radius14 + label, semua **klik-fungsional** (🖱️, lihat §6 untuk peta tujuan).
- [x] Footer: "Klik: OK · MENU: kembali".

#### FR-NEW-1 — Pilih Perangkat (RX/TX)
**File**: `screen_bt_devices.cpp` | **Status**: ✅ render, 🖱️ navigasi penuh, 🔵 BT nyata di hardware (v1.5) | **Tidak ada di 12 board asli**
- [x] **Di `sim/` (`KODE_OPTIMASI_SIM`)**: satu implementasi dipakai 2 kali (`ScreenId` `SCREEN_BT_DEVICES_RX`/`_TX`), daftar dummy diklik untuk pindah layar — RX: "Pixel 7 - Fadli", "iPhone - Sarah", "Laptop Kerja"; TX: "JBL Flip 6", "Sony SRS-XB13", "Speaker Mobil". Murni kosmetik, tidak ada radio BT di desktop.
- [x] **Di hardware (v1.5, `bt_backend.*`, lihat §3.8) — dua peran ini genuinely berbeda, bukan sekadar isi dummy yang beda**:
  - RX: **tidak ada daftar sama sekali** — sink itu pasif (ponsel yang connect ke dia, bukan sebaliknya) — cuma status: nama perangkat terhubung, atau "Menunggu ponsel terhubung...", + tombol "Lupakan".
  - TX: **scan nyata** — tombol "Scan" memicu classic-BT inquiry 10 detik, hasil muncul live, tap hasil → connect + simpan untuk auto-reconnect (`Preferences`).
- [x] Klik baris (🖱️, sim) atau klik hasil scan (hardware) → `SCREEN_RECEIVER`/`SCREEN_TRANSMITTER`. Badge "Terhubung" di layar tujuan (🖱️, ditambahkan di `screen_receiver.cpp`/`screen_transmitter.cpp`) → balik ke layar ini ("ganti perangkat") — round-trip nyata, diverifikasi via simulasi klik `lv_event_send` end-to-end tanpa crash (sim) dan via `pio run -e esp32dev` sukses link (hardware, §3.8).
- [x] Footer: "Klik untuk hubungkan - MENU kembali" (atau "MENU: kembali" saat kosong/RX status layar).
- [x] Menu's tile "BT Receiver"/"BT Transmitter" sekarang menuju ke sini dulu (bukan langsung ke FR-L4/FR-L5).

#### FR-L4 — BT Receiver
**File**: `screen_receiver.cpp` | **Status**: ✅ render, 🖱️ transport nyata, 🖱️ badge "Terhubung" → FR-NEW-1
- [x] Header ikon BLUETOOTH + "BT Receiver" + badge MENU + baterai.
- [x] Kartu hero ungu `COLOR_BT_HERO` 296×110px radius20, ikon bluetooth besar (18px), badge "Terhubung" (hijau+centang) pojok kanan-atas.
- [x] Caption judul/artis mengikuti track demo yang dipilih (3 pasangan, state persisten `s_track_idx`).
- [x] **Sengaja tanpa seek bar** — perangkat penerima tidak memiliki posisi transport (sumber dari HP).
- [x] Tombol prev/next (🖱️): ganti caption judul/artis (sim) / kirim AVRCP passthrough nyata ke ponsel terhubung — `bt_rx_next()`/`bt_rx_prev()` (hardware, v1.5, §3.8). Tombol play/pause (🖱️): toggle ikon (sim: state lokal) / `bt_rx_play_pause()` nyata (hardware) — tidak perlu engine audio lokal karena ini sinyal remote-control, bukan playback lokal.
- [x] **Hardware (v1.5)**: caption judul/artis jadi "Audio BT"/nama ponsel terhubung (`bt_rx_device_name()`) alih-alih 3 track demo; sumbernya `screen_bt_devices_rx_create()` memulai peran sink (`bt_backend_start_receiver()`, idempotent) begitu layar ini atau FR-NEW-1 RX dibuka.
- [x] Footer: "Audio dari ponsel via Bluetooth".

#### FR-L5 — BT Transmitter
**File**: `screen_transmitter.cpp` | **Status**: ✅ render, 🖱️ transport nyata, 🖱️ badge "Terhubung" → FR-NEW-1
- [x] Header BLUETOOTH + "BT Transmitter" + badge MENU + baterai.
- [x] Kartu hero ungu 296×92px radius20 dengan 3 ikon rantai sinyal `PLAY → NEXT → WIFI` (sumber → kirim → speaker) + badge "Terhubung".
- [x] Caption judul + posisi playlist (format "N / 18") mengikuti track demo yang dipilih (3 entri, state persisten `s_track_idx`).
- [x] **Punya seek bar** (beda dari L4 — perangkat ini sumber audio lokal): nilai demo 22% (statis — tidak di-tick seperti L1, karena layar ini tidak disambungkan ke `demo_cycle`'s elapsed timer).
- [x] Tombol prev/next (🖱️): ganti caption judul + posisi. Tombol play/pause (🖱️): toggle ikon PLAY/PAUSE (state lokal).
- [x] **Hardware (v1.5)**: transport tetap dekoratif (tidak ada sumber lokal nyata untuk dikontrol — lihat §3.8); tapi caption judul jadi nama speaker terhubung/tersimpan (`bt_tx_device_name()`) alih-alih 3 track demo, dan koneksi/scan-nya sendiri nyata lewat FR-NEW-1 TX (`bt_backend_start_transmitter()`, idempotent, dipanggil begitu layar ini atau FR-NEW-1 TX dibuka).
- [x] Footer: "Memutar lokal - keluar via speaker BT".

#### FR-L6 — Transfer File
**File**: `screen_transfer.cpp` | **Status**: ✅ render, tidak ada elemen interaktif yang diharapkan (layar status)
- [x] Header UPLOAD + "Transfer File" + badge MENU + baterai.
- [x] Kolom kiri: kartu "Hotspot ESP32" (SSID "DAP-ESP32", "Password: 12345678") + kartu "Buka di browser" (`http://192.168.4.1`, "atau http://dap.local").
- [x] Kolom kanan: kartu transfer "Aurora.mp3" / "2140 KB" + progress bar amber `COLOR_WARNING` (demo 60%) + "Menerima... · SD 29 GB".
- [x] Pill status: titik amber + "BT dan putar mati".
- [x] Footer: "MENU: menu / keluar".

#### FR-L7 — Overlay Alert (baterai rendah + SD error)
**File**: `overlay_alert.cpp` | **Status**: ✅ render, 🖱️ dialog interaktif, toast non-interaktif (by design)
- [x] **Modal baterai rendah** (`overlay_alert_show_low_battery(percent)`): backdrop gelap blocking-click `COLOR_OVERLAY_DIM` opacity 70%, dialog radius28 di tengah, ikon `BATTERY_EMPTY` merah + judul "Baterai rendah" + teks `"Tinggal N%. Colok USB ke D32 untuk mengisi daya."` + tombol "OK" (🖱️, klik → tutup, §6).
- [x] **Toast SD error** (`overlay_alert_show_sd_error()`): pita terbalik terang `COLOR_TOAST_BG`, teks gelap, radius8, "SD card tidak terbaca. Cek kartu dan kabel.", non-modal, auto-hilang 3000ms (§5.4).

#### FR-L8 — Beralih Mode
**File**: `screen_switch.cpp` | **Status**: ✅ render, tidak ada interaksi (by design — transisi otomatis)
- [x] Layar transisi **tanpa header/footer** (full-bleed).
- [x] Ikon tile ungu 84×84 radius28 (BLUETOOTH, melambangkan mode tujuan).
- [x] Teks "Beralih ke" / "BT Receiver" (18px).
- [x] Progress bar demo 35%.
- [x] Caption wrap: "ESP32 restart, tunggu sekitar 3 detik. Musik dan koneksi berhenti."

#### FR-L9/L10 — Volume HUD (berubah / senyap)
**File**: `overlay_volume.cpp` | **Status**: ✅ render 2 state, tidak blocking klik (by design)
- [x] Satu widget pill radius22 di `lv_layer_top()`, tampil di atas layar apa pun yang aktif.
- [x] State normal (`percent > 0`): ikon `VOLUME_MAX`, track hijau `COLOR_SUCCESS`.
- [x] State senyap (`percent <= 0`): ikon `MUTE`, track merah `COLOR_ERROR`, nilai 0.
- [x] Auto-hilang setelah 2000ms (§5.3).

#### FR-L11 — Daftar Playlist
**File**: `screen_playlists.cpp` | **Status**: ✅ render, 🖱️ baris interaktif
- [x] Header `AUDIO` + "Playlist" + badge MENU + baterai.
- [x] 5 baris dengan chip warna per kategori: Semua lagu (netral), **Favorit (biru, baris terpilih)**, Santai (ungu), Olahraga (coklat), Perjalanan (hijau) — masing-masing meta "N lagu · durasi".
- [x] Baris **klik-fungsional** (🖱️ → FR-L12, §6).
- [x] Footer: "Playlist dari /Playlists · Klik buka".

#### FR-L12 — Isi Playlist ("Favorit")
**File**: `screen_playlist_detail.cpp` | **Status**: ✅ render, 🖱️ back-chevron interaktif, 🪫 tombol aksi & baris belum
- [x] Header back-chevron (🖱️ → FR-L11, §6) + "Favorit" + badge MENU + baterai.
- [x] Kolom kiri: 3 tombol aksi — Putar (terisi `COLOR_ACCENT`), Acak & Ulang (outline) + meta "12 lagu" / "48 mnt".
- [x] Kolom kanan: 5 baris lagu — City Lights (sedang diputar, marker equalizer), Echoes, Aurora, Golden Road, Blue Hour.
- [ ] Klik tombol aksi / baris lagu untuk memutar — **belum ada handler** (🪫).
- [x] Footer: "Klik putar · MENU kembali".

---

## 5. Spesifikasi Animasi & Motion

Ini bagian inti yang diminta — **semua** gerakan/animasi/perilaku berbasis waktu di kodebase, tanpa kecuali.

### 5.1 Animasi reel kaset (vektor, `cassette_anim.cpp`) — satu-satunya animasi "custom vektor"

- **Larangan eksplisit dipatuhi**: tidak ada file gambar, tidak ada array frame bitmap/GIF. Reel dibangun dari widget LVGL murni.
- **Bentuk**: 2 reel identik, masing-masing = 1 lingkaran (`lv_obj`, diameter 28px, border 2px warna `COLOR_ON_ACCENT_DIM`, background transparan, `LV_RADIUS_CIRCLE`) berisi 1 **"jarum" vektor** (`lv_line` 2 titik, panjang = diameter reel, lebar garis 2px, warna sama) sebagai representasi spoke yang berputar.
- **Posisi**: reel kiri di `x = card_w/4 − 14px`, reel kanan di `x = 3·card_w/4 − 14px`, keduanya vertikal-tengah kartu hero. Di antaranya ada "jendela pita" — 1 `lv_obj` 56×20px radius8 warna `COLOR_CASSETTE_WINDOW`, ter-center di kartu.
- **Mekanisme rotasi**: `lv_obj_set_style_transform_angle()` pada objek garis, dikendalikan oleh **1 `lv_anim_t` per reel**:
  - Nilai: `0 → 3600` (satuan 0,1°, artinya 1 putaran penuh 360°).
  - Durasi: **4000 ms per putaran penuh**.
  - Repeat: `LV_ANIM_REPEAT_INFINITE`.
  - Pivot rotasi: `pivot_x = 14px` (tengah garis), `pivot_y = 1px` (karena tinggi garis ±2px).
- **Kedua reel berputar identik & serentak** — tidak ada variasi arah/kecepatan antar reel (penyederhanaan yang disengaja, bisa di-upgrade nanti kalau mau reel kiri/kanan beda arah seperti kaset asli).
- **Cadence render**: digerakkan oleh `guiTask` — `lv_tick_inc(delta)` + `lv_timer_handler()` dipanggil setiap **66 ms** (`vTaskDelay(pdMS_TO_TICKS(66))`), menghasilkan ±15 langkah animasi per detik, sesuai permintaan awal "15 FPS/interval 66ms". LVGL sendiri menghitung sudut berdasarkan waktu nyata (`lv_tick_inc`), bukan jumlah frame — jadi kecepatan rotasi (4 detik/putaran) tetap konsisten terlepas dari jitter frame, tapi *kehalusan visual* dibatasi oleh cadence 66 ms ini (~60 langkah diskret per putaran).
- **Pause/Resume** (`cassette_anim_set_spinning(bool)`):
  - Pause: `lv_anim_del(spoke, set_angle_cb)` — animasi dihapus total, sudut terakhir dibekukan apa adanya.
  - Resume: cek `lv_anim_get()`; kalau tidak ada animasi aktif, panggil ulang `start_spin()` — **animasi dimulai ulang dari sudut 0°**, bukan melanjutkan dari sudut terakhir (penyederhanaan yang disengaja & didokumentasikan — reel kosmetik, tidak kritikal untuk "resume tepat").
  - Trigger: dipanggil dari `now_playing_set_playing(bool)`. **Catatan penting**: tombol play/pause di L1 saat ini **tidak punya event handler klik** (lihat §6), jadi di build saat ini fungsi ini hanya terpanggil kalau ada kode integrasi lain yang memanggil API publiknya — secara visual di demo yang berjalan sekarang, reel **selalu berputar** selama layar Now Playing aktif.
- **Pembersihan memori**: saat `screen_manager_show()` berpindah dari L1 ke layar lain, `lv_obj_clean(root)` menghapus objek `card` beserta anak-anaknya (reel, garis) — LVGL otomatis menghapus `lv_anim_t` yang menempel pada objek yang dihapus. Tidak ada kebocoran anim/memori yang perlu dibersihkan manual.

### 5.2 Siklus waktu berjalan (elapsed-time tick, `demo_cycle.cpp`)

- **Timer independen** dari animasi reel (`lv_timer_create(demo_elapsed_tick_cb, 1000, NULL)`) — **inilah syarat eksplisit dari brief awal**: "update animasi kaset ... tidak mengganggu rendering teks durasi lagu."
- Jalan tiap **1000 ms**, **hanya aktif kalau layar aktif saat ini adalah L1** (`screen_manager_current() == SCREEN_NOW_PLAYING`; kalau tidak, fungsi langsung `return` tanpa efek).
- Logika: `elapsed_sec` mulai di 92 (= "1:32", cocok dengan label demo awal), naik 1 per tick, wrap ke 0 setelah 230 detik (= "3:50", total durasi demo). Memanggil `now_playing_set_progress(persen, "M:SS", "3:50")` yang meng-update **label teks + nilai `lv_bar`** — murni perubahan nilai widget (`LV_ANIM_OFF` pada `lv_bar_set_value`), **bukan animasi transisi visual** (tidak ada easing/tween saat progress bar berubah — nilainya langsung "lompat").

### 5.3 Overlay HUD Volume (`overlay_volume.cpp`) — widget top-layer, auto-hide bertimer

- Dipanggil via `overlay_volume_show(percent)`. Membuat 1 pill (`lv_layer_top()`, posisi `x=14,y=34`, lebar penuh−28px, tinggi 44px, radius22) dengan **shadow** (`lv_obj_set_style_shadow_width=10`, opacity 60%).
- **State warna** (bukan animasi gerak, tapi perubahan state visual instan):
  - `percent > 0` → ikon `VOLUME_MAX`, track isi warna `COLOR_SUCCESS` (hijau).
  - `percent <= 0` (senyap) → ikon `MUTE`, track isi warna `COLOR_ERROR` (merah), nilai 0.
- **Auto-hide**: `lv_timer_create(hide_timer_cb, 2000, NULL)` + `lv_timer_set_repeat_count(1)` — **sekali tembak**, setelah 2000 ms objek pill otomatis `lv_obj_del()` dan timer menghapus dirinya sendiri (`lv_timer_del(t)`).
- Kalau `overlay_volume_show()` dipanggil lagi sebelum timer lama habis, instance lama (pill + timer) dihapus dulu, lalu instance baru dibuat — **tidak ada animasi fade-in/fade-out**, murni create/delete langsung.
- **Tidak blocking klik** — pill tidak diberi `LV_OBJ_FLAG_CLICKABLE`, jadi klik pada layar di baliknya tetap tembus (sesuai desain modal vs non-modal FR-L7).

### 5.4 Overlay Alert (`overlay_alert.cpp`) — 2 pola timing berbeda

- **Modal baterai rendah** (`overlay_alert_show_low_battery(percent)`): **tidak auto-hide** — backdrop (`LV_OBJ_FLAG_CLICKABLE`, memblokir klik ke layar di bawahnya) + dialog bertahan sampai pengguna klik tombol "OK" (`LV_EVENT_CLICKED` → `dismiss_battery_dialog()` → `lv_obj_del()` pada backdrop, yang otomatis menghapus dialog & tombol OK sebagai anaknya).
- **Toast SD error** (`overlay_alert_show_sd_error()`): **auto-hide 3000 ms**, pola timer sama seperti volume HUD (`lv_timer_create(..., 3000, NULL)` + `repeat_count(1)`), tidak ada backdrop, tidak blocking.
- Sama seperti volume HUD: tidak ada animasi transisi masuk/keluar — murni create/delete instan.

### 5.5 Demo cycle timer (`demo_cycle.cpp`) — showcase otomatis non-navigasi

- `lv_timer_create(demo_cycle_overlays_cb, 12000, NULL)` — tiap 12 detik, bergantian memanggil `overlay_volume_show(67)` lalu `overlay_alert_show_sd_error()` (2 langkah, berulang). **Sengaja tidak** menyertakan dialog baterai-rendah di siklus otomatis ini — karena backdrop-nya blocking-click, memunculkannya tanpa diminta akan mengganggu pengguna yang sedang mencoba navigasi klik (bug yang ditemukan & diperbaiki — lihat §6).
- **Catatan desain berubah**: versi awal juga punya `lv_timer_create(demo_cycle_screens_cb, 4000, NULL)` yang **memaksa ganti layar** tiap 4 detik. Ini **sudah dihapus total** karena berkonflik dengan navigasi klik manual (pengguna klik tile Menu, 4 detik kemudian dipaksa pindah lagi oleh timer). Tidak ada lagi auto-cycle layar di kode saat ini.

### 5.6 Transisi antar-layar & perpindahan menu — detail lengkap apa yang nampak

**Jenis transisi**: hard cut instan. **Tidak ada** fade, slide, dissolve, zoom, atau efek visual apa pun saat berganti layar/menu — `screen_manager_show()` tidak memakai `lv_scr_load_anim()` dengan efek, hanya `lv_obj_clean()` + bangun ulang langsung (§3.2). Ini berlaku sama baik sebelum maupun sesudah perbaikan bug memori di §3.2 — yang berubah di sana hanya *urutan alokasi/pembersihan memori*, bukan tampilan transisinya.

**Urutan kejadian persis saat pengguna mengklik (contoh: tile "Daftar Lagu" di Menu)**:
1. Mouse/indev dibaca di dalam `lv_timer_handler()` (dipanggil `guiTask` tiap 66 ms) — klik baru terdeteksi paling lambat 66 ms setelah ditekan.
2. LVGL memicu `LV_EVENT_CLICKED` **secara synchronous**, masih di dalam pemanggilan `lv_timer_handler()` yang sama — `tile_clicked_cb()` langsung jalan.
3. Di dalam callback itu: `lv_obj_clean(s_root)` (hapus semua widget layar Menu) **lalu** `build_screen()` (bangun semua widget Daftar Lagu) — keduanya selesai total sebelum callback return. Tidak ada frame yang dirender di antara langkah ini.
4. LVGL baru menandai area yang berubah sebagai "invalid" dan merender ulang **setelah** urutan di atas selesai — artinya **kondisi layar kosong hasil `lv_obj_clean` tidak pernah benar-benar sampai ke panel**, tidak ada kedipan/flash hitam sesaat.
5. Karena seluruh root berubah, seluruh area 320×240 ditandai dirty → dengan partial draw buffer 15 baris, `disp_flush_cb` dipanggil berulang kali **di dalam siklus `lv_timer_handler()` yang sama** sampai seluruh layar baru terkirim ke panel, baru setelah itu `guiTask` lanjut ke `vTaskDelay(66ms)`.
6. **Hasil yang nampak ke pengguna**: dari klik sampai layar baru tampil utuh, jedanya ≤66 ms (satu siklus gui tick) + waktu transfer SPI — terasa sebagai **potongan instan**, bukan animasi geser/fade.

### 5.7 Efek hover/pressed pada elemen yang diklik — tidak ada sama sekali

Setiap widget yang dibuat lewat `ui_card_create` / `ui_button_create` (semua tombol, tile, badge, baris list) memanggil `lv_obj_remove_style_all()` tepat setelah objek dibuat. Ini menghapus **juga** style tema default LVGL (`lv_theme_default`, yang biasanya memberi efek redup/highlight saat ditekan). Konsekuensinya: **mengklik elemen apa pun di seluruh UI ini tidak memberi feedback visual pada elemen itu sendiri** — tidak ada perubahan warna, tidak ada efek redup/ripple saat ditekan. Satu-satunya tanda "klik berhasil" adalah efek akhirnya (layar berganti, overlay muncul, atau dialog tertutup) — bukan reaksi instan pada tombol yang ditekan.

### 5.8 Overlay bersifat independen dari transisi layar

`overlay_volume_show()` dan `overlay_alert_*()` membuat objeknya di `lv_layer_top()` — lapisan terpisah dari `s_root` yang dioperasikan `screen_manager`. Dampak yang nampak:
- Kalau HUD volume sedang tampil (dalam jendela 2 detik auto-hide-nya, §5.3) lalu pengguna mengklik navigasi ke layar lain, **HUD tetap melayang di atas layar baru tanpa terganggu** — `lv_obj_clean(s_root)` tidak pernah menyentuh `lv_layer_top()`.
- Dialog modal baterai-rendah (blocking-click, §5.4) juga tetap menutupi layar apa pun yang aktif sampai tombol "OK" diklik — termasuk kalau secara teori layar di baliknya berganti saat dialog masih terbuka (tidak bisa terjadi lewat UI saat ini karena dialog memblokir semua klik lain, tapi tetap berlaku kalau dipicu dari kode lain).

### 5.9 Gap yang ditemukan: FR-L8 (Beralih Mode) saat ini tidak bisa diakses

`SCREEN_SWITCH` **tidak pernah dipanggil oleh kode apa pun saat ini** — satu-satunya referensinya ada di `switch-case` dispatcher `screen_manager.cpp`; `grep SCREEN_SWITCH` di semua `.cpp` lain tidak menemukan pemanggilan `screen_manager_show(SCREEN_SWITCH)`. Dulu layar ini terlihat lewat timer auto-cycle (`demo_cycle_screens_cb`) yang sudah dihapus (§5.5) karena mengganggu navigasi klik manual. Akibatnya: **kode FR-L8 benar dan lengkap, tapi tidak "reachable" dari UI mana pun saat ini** — baik di `sim/` maupun (kalau nanti ada indev) di hardware. Perlu salah satu: (a) rute klik baru menuju FR-L8 ditambahkan untuk demonstrasi, atau (b) dipicu otomatis nanti oleh logika ganti-mode operasi yang nyata (sesuai tujuan aslinya: layar transisi saat ESP32 restart pindah mode) — lihat §9.

---

## 6. Navigasi & Interaksi — apa yang **nyata jalan** vs dekoratif

### 6.1 Diagram alur navigasi (klik nyata, hanya berfungsi di `sim/`)

```mermaid
flowchart LR
    Menu["FR-L3 Menu Mode"]
    NowPlaying["FR-L1 Now Playing"]
    Library["FR-L2 Daftar Lagu"]
    Receiver["FR-L4 BT Receiver"]
    Transmitter["FR-L5 BT Transmitter"]
    Transfer["FR-L6 Transfer File"]
    Playlists["FR-L11 Daftar Playlist"]
    Detail["FR-L12 Isi Playlist"]
    VolumeHUD(["FR-L9/L10 Volume HUD<br/>(overlay, tanpa blocking)"])
    AlertModal(["FR-L7 Dialog Baterai Rendah<br/>(overlay, blocking)"])
    AlertToast(["FR-L7 Toast SD Error<br/>(overlay, auto-hilang)"])

    Menu -->|klik tile| Library
    Menu -->|klik tile| Playlists
    Menu -->|klik tile| NowPlaying
    Menu -->|klik tile| Receiver
    Menu -->|klik tile| Transmitter
    Menu -->|klik tile| Transfer
    Menu -->|klik "Kembali"| NowPlaying

    NowPlaying -->|klik badge MENU| Menu
    Library -->|klik badge MENU| Menu
    Receiver -->|klik badge MENU| Menu
    Transmitter -->|klik badge MENU| Menu
    Transfer -->|klik badge MENU| Menu
    Playlists -->|klik badge MENU| Menu
    Detail -->|klik badge MENU| Menu

    Playlists -->|klik baris| Detail
    Detail -->|klik back-chevron| Playlists

    VolumeHUD -.->|tampil di atas layar manapun yang aktif| Menu
    AlertModal -.->|tampil di atas layar manapun yang aktif| Menu
    AlertToast -.->|tampil di atas layar manapun yang aktif| Menu
```

`FR-L8 Beralih Mode` tidak digambar sebagai node bertautan — layar ini murni transisi otomatis (tidak dipicu klik, tidak punya tujuan navigasi keluar yang diklik pengguna).

### 6.2 Peta elemen interaktif

Tabel ini presisi berdasarkan `grep add_event_cb` di seluruh kodebase — **hanya 6 file** yang benar-benar punya handler klik:

| Elemen | File | Aksi klik |
|---|---|---|
| Badge "MENU" (semua layar berbadge) | `ui_common.cpp` | → `screen_manager_show(SCREEN_MENU)` |
| 7 tile di Menu | `screen_menu.cpp` | → layar sesuai tile (lihat FR-L3) |
| Baris di Daftar Playlist | `screen_playlists.cpp` | → `SCREEN_PLAYLIST_DETAIL` |
| Back-chevron di Isi Playlist | `screen_playlist_detail.cpp` | → `SCREEN_PLAYLISTS` |
| Tombol "OK" di dialog baterai rendah | `overlay_alert.cpp` | Tutup dialog |

**Yang terlihat seperti tombol tapi TIDAK punya handler (murni dekoratif, hanya `LV_OBJ_FLAG_CLICKABLE` bawaan dari `ui_button_create` tanpa callback)**: tombol prev/play-pause/next di FR-L1 (Now Playing), FR-L4 (Receiver), FR-L5 (Transmitter); 3 tombol aksi (Putar/Acak/Ulang) di FR-L12; baris lagu di FR-L2 (Daftar Lagu). Mengklik elemen-elemen ini di `sim/` **tidak melakukan apa-apa** saat ini.

### 6.3 Input device

- `sim/sim_main.cpp` — mendaftarkan mouse SDL sebagai `lv_indev` (`LV_INDEV_TYPE_POINTER`), skala dibagi `ZOOM=2` sesuai ukuran window 640×480 yang merender kanvas 320×240. **Ini yang membuat navigasi di atas benar-benar bisa diklik di desktop.**
- `kode-optimasi.ino` (hardware asli) — **tidak mendaftarkan `lv_indev` apa pun**. Jadi di ESP32 fisik, semua handler klik di atas ada di kode tapi tidak bisa dipicu sampai ada driver touch (XPT2046, seperti di `final-form`) atau tombol fisik yang didaftarkan sebagai indev.

### 6.4 Riwayat bug yang ditemukan & diperbaiki

Lewat pengujian `sim/`, bukan review statis:
1. `lv_obj_set_style_margin_top` dipanggil padahal fungsi ini **tidak ada** di LVGL 8.x → diganti `lv_obj_set_style_pad_top` di `screen_switch.cpp` & `overlay_alert.cpp`.
2. `screen_manager_show()` membangun layar baru sebelum menghapus layar lama → RAM dobel sesaat → segfault di `sim/` (pool 24 KB sengaja kecil) → diperbaiki jadi `lv_obj_clean()` dulu (§3.2).
3. Tidak ada `lv_indev` + tidak ada `add_event_cb` sama sekali di awal → UI terlihat benar tapi 100% tidak bisa dinavigasi → ditambahkan mouse indev di sim + handler klik riil (§6.2) + dihapus timer auto-cycle layar yang berkonflik (§5.5).

---

## 7. Spesifikasi Teknis & Verifikasi

### 7.1 Target hardware
- ESP32 DevKit V1 (WROOM-32, **tanpa PSRAM**).
- Panel ILI9341 (default) atau ST7789 (ganti 1 define di `TFT_eSPI_User_Setup.h`), SPI 320×240 landscape (`tft.setRotation(1)`).
- Pin SPI bersama TFT+SD+touch, disalin dari `final-form/pins.h` (hardware fisik yang sama): `SCK=18, MOSI=23, MISO=19`, `TFT_CS=17, TFT_DC=27, TFT_RST=33`.

### 7.2 Dependensi
- `lvgl` ^8.3 (diuji konkret dengan 8.4.0).
- `TFT_eSPI` ^2.5 (diuji konkret dengan 2.5.43).
- Build Arduino IDE **atau** PlatformIO (`platformio.ini` disediakan, `src_dir=.`, `build_src_filter` mengecualikan `sim/`).
- PlatformIO butuh 2 flag ekstra yang **tidak jelas dari dokumentasi LVGL** (ditemukan lewat percobaan build nyata): `-D LV_CONF_INCLUDE_SIMPLE=1` dan `-I.` agar `lv_conf.h` lokal proyek ditemukan oleh paket `lvgl/lvgl` generik.

### 7.3 Verifikasi yang sudah dijalankan (bukan klaim, tapi hasil build/run nyata)
1. `pio run` (`esp32dev`, framework Arduino) — **sukses**, RAM 17,8%, Flash 62,8% (sebelum backend BT, §3.8 ditambahkan).
2. `sim/make sim` — clone `lvgl` asli (bukan stub) via git, compile `.c` LVGL dengan `gcc` (bukan `g++`, karena `g++` menolak kode C valid seperti konversi implisit `void*`→`T*`), link dengan sumber `.cpp` aplikasi asli tanpa modifikasi.
3. `sim/./sim` dijalankan di bawah `gdb` selama 60–180 detik tanpa crash, melewati banyak siklus overlay + navigasi klik manual.
4. Screenshot/analisis manual tidak dilakukan (tooling screenshot headless tidak tersedia di environment ini) — verifikasi visual terakhir tetap perlu dilakukan manual oleh pengguna di layar sim atau hardware asli.
5. **v1.5**: `pio run -e esp32dev` dengan `bt_backend.cpp` + lib `ESP32-A2DP` ter-link — percobaan pertama **overflow flash** (126,7% dari partisi default, stack classic-BT besar), diperbaiki dengan `board_build.partitions = huge_app.csv`; rebuild setelah fix **sukses**, RAM 26,5% (86,7 KB), Flash 52,8% (1,66 MB dari 3 MB). `sim/make sim` diulang setelah perubahan ini dan tetap sukses (file BT tidak ikut di-link ke sim, by design).

### 7.4 Prinsip desain yang dipegang konsisten
- **Tidak ada bitmap/raster untuk animasi atau ikon** — vektor (`lv_line`+transform) atau glyph font bawaan saja.
- **Satu task pemilik LVGL** (`guiTask`) — tidak ada mutex di kodebase karena tidak dibutuhkan.
- **Data demo literal dari mockup**, bukan placeholder generik ("Lorem ipsum" dsb) — supaya hasil visual langsung bisa dibandingkan ke `dap-ui-landscape/`.

---

## 8. Risiko & Limitasi Dikenal

| Risiko/Limitasi | Dampak | Status |
|---|---|---|
| Tidak ada indev di hardware asli | Navigasi klik yang sudah dibuat tidak bisa dipicu di device fisik | Disengaja, di luar lingkup awal |
| Tombol transport (prev/play/next) di FR-L1/FR-L5, tombol aksi di FR-L12, baris di FR-L2 belum punya handler nyata | Terlihat seperti bisa diklik tapi tidak melakukan apa-apa (kecuali FR-L4/BT Receiver, yang sejak v1.5 nyata lewat AVRCP passthrough — §3.8) | Belum dikerjakan — lihat §9 |
| `now_playing_set_battery()` adalah stub no-op | Baterai header selalu statis 78%, tidak bisa diubah lewat API publik | Didokumentasikan sebagai `ponytail:` di kode |
| Resume animasi reel mulai dari 0°, bukan posisi terakhir | Efek visual minor, tidak terlihat jelas karena rotasi simetris | Disengaja, kosmetik saja |
| Font bold tidak tersedia (LVGL bawaan) | Judul lagu pakai regular 18px, bukan bold | Perlu font custom via `lv_font_conv` kalau mau bold asli |
| `LV_MEM_SIZE` 24 KB belum diuji dengan worst-case nyata (overlay + layar kompleks bersamaan) | Risiko OOM di kombinasi tertentu belum 100% tereliminasi di hardware asli | Perlu uji nyata di device, bukan hanya sim |
| Tidak ada SD/audio/WiFi nyata | Semua data durasi/SSID/IP/transfer adalah teks statis | Disengaja, lingkup "UI shell" |
| BT klasik nyata (v1.5) tapi sumber A2DP kirim silence | Pairing/scan/AVRCP bekerja sungguhan, tapi belum ada musik sungguhan mengalir (butuh SD+decoder, di luar lingkup modul ini) | Disengaja — lihat §3.8, §9 |
| **FR-L8 (Beralih Mode) tidak reachable dari UI mana pun** (§5.9) | Layar transisi ganti-mode sudah lengkap di kode tapi tidak pernah terpanggil — tidak bisa dilihat/diuji lewat navigasi biasa | Regresi dari penghapusan auto-cycle layar (§5.5) — perlu rute klik baru atau trigger dari logika ganti-mode nyata |
| Tidak ada efek hover/pressed di elemen apa pun (§5.7) | Klik terasa "tanpa respons instan" sebelum efek akhirnya muncul (ganti layar/overlay) | Disengaja (konsekuensi dari `lv_obj_remove_style_all` di semua factory), bisa ditambah style pressed kalau UX dirasa kurang responsif |

---

## 9. Roadmap (di luar lingkup modul ini, untuk pass integrasi berikutnya)

1. **Input nyata di hardware**: tambahkan `lv_indev` touch (XPT2046, seperti `final-form`) atau tombol fisik ke `kode-optimasi.ino`, agar handler klik yang sudah ada di §6 bisa dipicu di device.
2. **Lengkapi handler klik yang masih kosong**: tombol transport FR-L1/FR-L5 (toggle play/pause nyata, panggil `now_playing_set_playing()`), baris FR-L2 (play lagu), 3 tombol aksi FR-L12. (FR-L4/BT Receiver sudah nyata sejak v1.5, §3.8.)
3. **Integrasi backend nyata yang masih tersisa**: ganti data demo dengan pembacaan SD card (daftar lagu/playlist), status WiFi AP nyata, level baterai ADC nyata (lewat `now_playing_set_battery()` yang sudah ada API-nya tapi masih stub). BT A2DP klasik (scan/pair/AVRCP) **sudah nyata sejak v1.5** (`bt_backend.*`, §3.8) — yang tersisa cuma menyambungkan payload audio sumbernya ke SD+decoder nyata (sekarang kirim silence).
4. **Isi `audioTaskStub`**: pipeline decode/output I2S nyata di core 0, sesuai kerangka yang sudah disiapkan.
5. **Font bold asli** (opsional) kalau kesetiaan visual terhadap spek awal dianggap penting.
6. **Buat FR-L8 bisa diakses lagi** (§5.9): tambah rute klik demonstrasi (mis. dari tombol BT di Menu) atau sambungkan ke logika ganti-mode operasi nyata saat itu dibangun.
7. **Pertimbangkan efek pressed/hover minimal** (§5.7) kalau umpan balik visual instan saat klik dirasa perlu, sebelum menambah input fisik di hardware (poin 1) — supaya pengguna tahu klik terdeteksi walau hasil akhirnya butuh ≤66 ms untuk tampil.
