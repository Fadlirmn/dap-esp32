# Plan: DAP / Audio Receiver-Transmitter berbasis ESP32 (Lolin D32)

## 1. Tujuan

DAP portabel dengan layar 2.8" ILI9341 (touch resistif + slot SD), DAC I2S PCM5102A (jack 3.5 mm ke IEM), rotary encoder sebagai kontrol utama, transfer file nirkabel lewat WiFi, indikator baterai, dan casing hasil 3D print. Inspirasi bentuk: FiiO Snowsky Echo Mini.

## 2. Komponen

| Komponen | Fungsi |
| --- | --- |
| Lolin D32 (ESP32-WROOM-32, 4 MB flash, 57 x 25.4 mm) | MCU, WiFi, Bluetooth Classic, charger LiPo + konektor JST PH 2.0 bawaan |
| Modul TFT 2.8" ILI9341 + XPT2046 + slot SD | Layar sentuh resistif dan penyimpanan lagu (SPI) |
| PCM5102A | DAC I2S, output analog ke IEM |
| Rotary encoder dengan tombol klik (KY-040) | Volume (putar), play/pause dan OK (klik) |
| 3 tact switch 6x6 mm (PREV, NEXT, MENU) + 2 resistor pull-up 10k | Tombol navigasi di sisi atas, gaya Snowsky |
| Baterai LiPo 3.7V berproteksi, konektor JST PH 2.0 | Sumber daya |
| Kapasitor 100-470 uF, 100 nF | Peredam lonjakan arus, debounce encoder |
| Baut M2.5 x 20 (4), double tape | Casing |

Tidak lagi dipakai (karena D32): modul IP5306 / TP4056, boost converter 5V, dan resistor pembagi tegangan baterai eksternal.

## 3. Lolin D32: yang perlu diketahui

- Charger TP4054 (maks 500 mA), baterai ke JST PH 2.0. Saat USB terpasang, sistem dipasok USB dan baterai di-charge; tanpa USB, baterai memasok board.
- Regulator 3.3V ME6211 (kapasitas arus terbatas, sekitar 500 mA menurut spesifikasi umumnya; cek datasheet). Backlight + WiFi/Bluetooth + DAC bisa mendekati batas, jadi perhatikan arus puncak.
- GPIO 35 tersambung internal ke pembagi baterai 100k/100k (tidak di header). Jangan dipakai untuk hal lain.
- GPIO 5 = LED onboard dan strapping, dibiarkan kosong. Satu micro-USB untuk flash dan charge.
- Tidak ada proteksi baterai di board: pakai baterai berproteksi dan cek polaritas JST sebelum colok.
- Tidak ada PSRAM.

## 4. Mode operasi

| Mode | Catatan |
| --- | --- |
| Local Player (SD → IEM) | MP3/WAV/FLAC/AAC lewat ESP32-audioI2S |
| BT Receiver (HP → IEM) | A2DP Sink, codec SBC |
| BT Transmitter (SD → headset BT) | A2DP Source, eksperimental, hanya MP3 44.1 kHz |
| Transfer File (WiFi) | Web upload ke /Music, BT dan playback nonaktif |

Satu mode aktif per boot. Ganti mode lewat menu (tahan tombol), lalu ESP32 restart.

## 5. Pinout

### PCM5102A (I2S)

| Pin modul | Koneksi |
| --- | --- |
| VIN | 3V3 |
| GND | GND |
| BCK | GPIO 26 |
| LCK (LRCK/WS) | GPIO 25 |
| DIN | GPIO 22 |
| SCK | GND |
| FLT | GND |
| DEMP | GND |
| XSMT | 3V3 (wajib) |
| FMT | GND |

### SPI bersama (TFT, touch, SD)

| Sinyal | GPIO |
| --- | --- |
| SCK | 18 |
| MOSI | 23 |
| MISO | 19 |
| TFT CS | **17** (dipindah dari 5) |
| SD CS | 4 |
| Touch CS | 21 |
| TFT DC | 27 |
| TFT RST | 33 |
| Touch IRQ | 34 (tidak dipakai di firmware) |
| TFT LED | 3V3 (atau GPIO 32 untuk atur kecerahan) |

### Tombol atas (aktif LOW ke GND)

| Tombol | GPIO |
| --- | --- |
| PREV | 32 (pull-up internal) |
| NEXT | 36 (input only, pull-up eksternal 10k ke 3V3) |
| MENU | 39 (input only, pull-up eksternal 10k ke 3V3) |

### Encoder dan baterai

| Sinyal | Koneksi |
| --- | --- |
| Encoder CLK / DT / SW | GPIO 13 / 14 / 16 |
| Baterai | JST PH 2.0 di D32, VBAT sense internal GPIO 35 |

## 6. Anggaran GPIO

18 GPIO terpakai (I2S 3, SPI 3, CS 3, kontrol TFT 3, encoder 3, tombol atas 3) dari 26 yang terekspos. Backlight langsung ke 3V3 (PWM tidak tersedia). Hindari 0, 2, 5, 12, 15 (strapping) dan 1, 3 (UART0).

## 7. Kontrol

Putar encoder hanya untuk volume. Navigasi lewat tombol atas. Touch resistif jadi cadangan (`ENABLE_TOUCH 0` mematikannya).

| Kontrol | Daftar lagu / menu | Memutar |
| --- | --- | --- |
| Putar encoder | volume | volume |
| Klik encoder | pilih / putar | play / pause |
| Klik 2x encoder | - | lagu berikutnya |
| PREV / NEXT | naik / turun (tahan = cepat) | lagu sebelum / berikut |
| MENU | buka menu / kembali | buka menu |

## 8. Transfer file dan baterai

- Web upload di ESP32 (AP hotspot atau WiFi rumah). Hentikan playback dan Bluetooth saat transfer. FTP tidak dipakai.
- Persen baterai dari GPIO 35 (VBAT = 2 x tegangan pin), kurva Li-ion: 4.2 V ≈ 100%, 3.7 V ≈ 35%, 3.3 V ≈ 0% (perkiraan kasar). D32 tidak punya pin status charger, hanya LED charge.

## 9. Casing 3D print

Parametrik di `dap_case.scad`, 3 bagian (base, shelf, top), ukuran luar 104 x 64 x 28 mm.

- Susunan bawah ke atas: baterai, rak (D32 + PCM5102A), layar 2.8", bezel.
- Lubang: micro-USB D32 (kiri, flash + charge), jack 3.5 mm (kiri), encoder (kanan, di garis sambungan), 3 tombol PREV/NEXT/MENU (dinding atas, X = 44/56/68), saklar power opsional.
- Bagian ke-4 `btn.stl`: 3 tutup tombol cetak untuk tact switch yang dilem di rak.
- Tanpa support, 4 baut M2.5 x 20 dari bawah. Cetak `top` dulu untuk uji pas layar.
- Ukuran PCB layar, area aktif, dan baterai adalah perkiraan: ukur ulang dengan jangka sorong.
- Edit lewat OpenSCAD Playground (ochafik.com/openscad2).

## 10. Firmware

Project PlatformIO (`esp32-dap-firmware.zip`): Local, Receiver, Transmitter, Transfer. Library: TFT_eSPI, ESP32-audioI2S, ESP32-A2DP, arduino-audio-tools, arduino-libhelix. Partisi `huge_app.csv`. Belum dikompilasi atau diuji di hardware.

## 11. Tahap pengerjaan

- [ ] 1. Uji PCM5102A + sinyal sinus/WAV dari SD (tanpa layar)
- [ ] 2. Tambahkan layar dan touch, UI dasar
- [ ] 3. Aktifkan mode A2DP Sink
- [ ] 4. Aktifkan mode A2DP Source dan mode switch
- [ ] 5. Pasang rotary encoder (volume), tombol atas PREV/NEXT/MENU, dan kalibrasi touch
- [ ] 6. Mode Transfer File lewat WiFi
- [ ] 7. Colok baterai LiPo ke JST D32 dan aktifkan indikator baterai
- [ ] 8. Cetak casing (uji pas top dulu) dan rakit
- [ ] 9. Optimasi buffer dan daya tahan

## 12. Risiko

| Risiko | Mitigasi |
| --- | --- |
| Regulator 3.3V D32 kewalahan saat arus puncak | Turunkan backlight, hindari WiFi + backlight terang bersamaan, cek datasheet |
| Polaritas JST baterai terbalik | Cek + / - sebelum colok |
| Tombol NEXT/MENU tidak terbaca | GPIO 36/39 tanpa pull-up internal: pasang resistor 10k ke 3V3 |
| Audio stuttering saat SD → Bluetooth | Perbesar buffer, kurangi beban UI (tanpa PSRAM) |
| Layar atau SD saling mengganggu di SPI | Kelola CS, atur kecepatan SPI per perangkat |
| Noise/derak di output | Kapasitor dekat 3V3, pisahkan GND audio, kabel I2S pendek |
| DAC mute tanpa sebab | XSMT harus 3V3 |
| Ukuran casing tidak pas | Ukur komponen, cetak top untuk uji pas |

## 13. Carrier board

PCB 2 layer 98.8 x 58.8 mm menggantikan rak cetak: D32, PCM5102A, 3 tact switch, pull-up, dan kapasitor dipasang di PCB. Kabel tersisa untuk layar (J_LCD 14 + J_SD 4), encoder (5), dan baterai ke JST D32. Detail di `carrierboard.zip` (netlist, outline DXF, penempatan). Footprint dan urutan pin modul harus diverifikasi dengan komponen asli sebelum memesan.
