# DAP ESP32 (firmware)

Firmware PlatformIO untuk DAP: Local Player, BT Receiver, BT Transmitter, dan Transfer File (WiFi).
Untuk board **Lolin D32** (charger LiPo bawaan, baterai ke konektor JST PH 2.0). Pin mengikuti `plan.md` (lihat `src/config.h`; pin TFT/touch ada di `platformio.ini`).
Perubahan dari versi DevKit V1: TFT CS pindah ke GPIO 17. Pembacaan baterai memakai GPIO 35 yang sudah tersambung ke pembagi 100k/100k di board, jadi JANGAN pasang pembagi tegangan eksternal.

## Cara pakai
1. Buka folder ini di VS Code + PlatformIO, lalu `Upload`.
2. Isi `src/config.h`: `BT_TARGET_NAME` (nama headset untuk mode Transmitter), `WIFI_STA_SSID/PASS` (opsional).
3. Boot pertama: layar minta kalibrasi touch (tekan 4 sudut). Untuk kalibrasi ulang, tahan tombol encoder saat menyalakan.
4. Taruh musik di folder `/Music` pada SD card (FAT32), atau pakai mode Transfer File.

## Kontrol
Putar encoder = volume saja. Navigasi lewat tombol di sisi atas (gaya Snowsky).

| Kontrol | Daftar lagu / menu | Memutar |
| --- | --- | --- |
| Putar encoder | volume | volume |
| Klik encoder | pilih / putar | play / pause |
| Klik 2x encoder | - | lagu berikutnya |
| PREV / NEXT | naik / turun (tahan = cepat) | lagu sebelum / berikut |
| MENU | buka menu | buka menu |

Tombol: PREV GPIO 32, NEXT GPIO 36, MENU GPIO 39 (ke GND saat ditekan). GPIO 36 dan 39 input-only: pasang resistor pull-up 10k ke 3V3.
Layar sentuh tetap cadangan; `ENABLE_TOUCH 0` di `config.h` mematikannya.

## Ganti mode
Tekan MENU (atau tahan encoder) -> pilih mode. Mode disimpan lalu ESP32 restart. Satu mode aktif per boot,
karena Bluetooth, WiFi, dan decoder audio tidak muat/aman jalan bersamaan.

## Catatan
- Belum dikompilasi atau diuji di hardware. Versi library bisa mempengaruhi API; kalau ada error compile, sesuaikan di bagian yang disebut.
- Mode Transmitter eksperimental: hanya MP3 44.1 kHz.
- Pin touch IRQ (GPIO 34) tidak dipakai di kode (touch di-polling).
- Header menampilkan "USB" bila tegangan baterai terbaca di bawah 2.5 V. Pada D32 dengan USB terpasang tanpa baterai, pembacaan bisa tetap tinggi, jadi indikator bisa salah.
- D32 tidak punya pin status charger; hanya LED charge di board.
