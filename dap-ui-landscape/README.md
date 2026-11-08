# DAP UI landscape 320x240 (Material)

- `ui-preview.html`: 12 layar landscape dalam satu halaman, ukuran asli 320x240 (buka di browser; font Roboto butuh internet).
- `source/`: file asli canvas (`canvas.json`, `L_*.dc.html`). Hanya bisa dirender di editor Design, jadi untuk arsip.
- Desain online: https://claude.ai/artifact/Bq3bm5KEzP8QeHG6o17VTZ

Layar: Now Playing, Daftar Lagu, Menu mode, BT Receiver, BT Transmitter, Transfer File, Baterai rendah dan error,
Beralih mode, Volume berubah, Volume senyap, Daftar playlist, Isi playlist.

Catatan: sudah diterapkan ke firmware (`final-form/`) -- ini desain acuan yang
dipakai, bukan `dap-ui-design/` (portrait, lama, sudah dihapus). Sampul
ilustrasi kaset di mockup ini diganti sampul album asli di firmware: panel
Now Playing (Local Player & BT Transmitter) menampilkan `/tracks/<nama>.raw`
(94x94, RGB565) kalau ada, jatuh ke ikon nada kalau tidak -- BT Receiver
tetap pakai ikon bluetooth (tidak ada file lokal untuk ditampilkan). Playlist
diasumsikan dari file .m3u di /Playlists.
