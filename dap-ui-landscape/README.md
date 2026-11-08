# DAP UI landscape 320x240 (Material)

- `ui-preview.html`: 12 layar landscape dalam satu halaman, ukuran asli 320x240 (buka di browser; font Roboto butuh internet).
- `source/`: file asli canvas (`canvas.json`, `L_*.dc.html`). Tidak butuh editor Design untuk tampil -- bagian yang terlihat cuma HTML/CSS/SVG statis. `screenshots/` adalah hasil render tiap file lewat `google-chrome-stable --headless --screenshot` pada ukuran asli 320x240, dipakai di galeri bawah.
- Desain online: https://claude.ai/artifact/Bq3bm5KEzP8QeHG6o17VTZ

## Galeri

| | | |
|---|---|---|
| ![Now Playing](screenshots/01-now-playing.png)<br>1. Now Playing | ![Daftar Lagu](screenshots/02-daftar-lagu.png)<br>2. Daftar Lagu | ![Menu mode](screenshots/03-menu-mode.png)<br>3. Menu mode |
| ![BT Receiver](screenshots/04-bt-receiver.png)<br>4. BT Receiver | ![BT Transmitter](screenshots/05-bt-transmitter.png)<br>5. BT Transmitter | ![Transfer File](screenshots/06-transfer-file.png)<br>6. Transfer File |
| ![Baterai rendah dan error](screenshots/07-overlay.png)<br>7. Baterai rendah dan error | ![Beralih mode](screenshots/08-beralih-mode.png)<br>8. Beralih mode | ![Volume berubah](screenshots/09-volume.png)<br>9. Volume berubah |
| ![Volume senyap](screenshots/10-volume-list.png)<br>10. Volume senyap | ![Daftar playlist](screenshots/11-daftar-playlist.png)<br>11. Daftar playlist | ![Isi playlist](screenshots/12-isi-playlist.png)<br>12. Isi playlist |

Catatan: sudah diterapkan ke firmware (`final-form/`) -- ini desain acuan yang
dipakai, bukan `dap-ui-design/` (portrait, lama, sudah dihapus). Sampul
ilustrasi kaset di mockup ini diganti sampul album asli di firmware: panel
Now Playing (Local Player & BT Transmitter) menampilkan `/tracks/<nama>.raw`
(94x94, RGB565) kalau ada, jatuh ke ikon nada kalau tidak -- BT Receiver
tetap pakai ikon bluetooth (tidak ada file lokal untuk ditampilkan). Playlist
diasumsikan dari file .m3u di /Playlists.
