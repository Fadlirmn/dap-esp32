# DAP UI 240x320 (Material)

- `ui-preview.html`: semua layar dalam satu halaman (buka di browser; font Roboto butuh internet, tanpa itu memakai font sans-serif bawaan).
- `source/`: file asli canvas (`canvas.json` dan `*.dc.html`). File `.dc.html` tidak butuh runtime Design untuk tampil -- bagian yang terlihat cuma HTML/CSS/SVG statis, cuma skrip editornya yang butuh runtime itu. `screenshots/` adalah hasil render tiap file lewat `google-chrome-stable --headless --screenshot` pada ukuran asli 240x320, dipakai di bawah ini.
- Desain online: https://claude.ai/artifact/Bq3bm5KEzP8QeHG6o17VTZ (pribadi sampai kamu membagikannya).

Catatan: desain belum diterapkan 1:1 ke firmware -- firmware pakai landscape 320x240 (bukan portrait 240x320 di sini), jadi tiap layar direinterpretasi ke grid/geometri landscape, bukan dicopy mentah. Playlist diasumsikan dari file .m3u di /Playlists.

## Layar

| | | |
|---|---|---|
| ![Now Playing](screenshots/01-now-playing.png)<br>1. Now Playing | ![Daftar Lagu](screenshots/02-daftar-lagu.png)<br>2. Daftar Lagu | ![Menu mode](screenshots/03-menu-mode.png)<br>3. Menu mode |
| ![BT Receiver](screenshots/04-bt-receiver.png)<br>4. BT Receiver | ![BT Transmitter](screenshots/05-bt-transmitter.png)<br>5. BT Transmitter | ![Transfer File](screenshots/06-transfer-file.png)<br>6. Transfer File |
| ![Overlay](screenshots/07-overlay.png)<br>7. Overlay (baterai rendah, SD error) | ![Beralih mode](screenshots/08-beralih-mode.png)<br>8. Beralih mode | ![Volume](screenshots/09-volume.png)<br>9. Indikator volume |
| ![Volume senyap](screenshots/10-volume-list.png)<br>10. Indikator volume (senyap) | ![Daftar playlist](screenshots/11-daftar-playlist.png)<br>11. Daftar playlist | ![Isi playlist](screenshots/12-isi-playlist.png)<br>12. Isi playlist |
