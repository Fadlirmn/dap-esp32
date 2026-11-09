# DAP-Esp32Devkit

![ESP32](https://img.shields.io/badge/ESP32-E7352C?style=flat&logo=espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00979D?style=flat&logo=arduino&logoColor=white)
![Bluetooth](https://img.shields.io/badge/Bluetooth%20A2DP-0082FC?style=flat&logo=bluetooth&logoColor=white)
![SD Card](https://img.shields.io/badge/SD%20Card-FFC107?style=flat&logo=sdcard&logoColor=black)
![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=cplusplus&logoColor=white)

ESP32-based portable audio player (SD card → Bluetooth A2DP source), with a
rotary encoder + button UI on an ILI9341 display.

<p align="center">
  <img src="kode-optimasi/screenshots/01-now-playing.png" width="200" alt="Now Playing">
  <img src="kode-optimasi/screenshots/03-menu-mode.png" width="200" alt="Menu mode">
  <img src="kode-optimasi/screenshots/08-daftar-playlist.png" width="200" alt="Daftar playlist">
</p>
<p align="center"><em>LVGL UI preview (<code>kode-optimasi/</code>) — more screenshots in <a href="kode-optimasi/README.md">kode-optimasi/README.md</a></em></p>

## Layout

- **`kode-optimasi/`** — the firmware, and the only project folder in this
  repo. LVGL 8.x UI (13 screens) + real BT A2DP source/sink scan/pair/AVRCP,
  MP3 decode + I2S output, WiFi AP file transfer, physical rotary encoder +
  3-button input, and XPT2046 touch. Has its own desktop SDL preview
  (`sim/`) and screenshot gallery — see `kode-optimasi/README.md`.

`final-form/` (the earlier, separate firmware) and the `old/`/
`dap-ui-landscape/`/`docs/` reference material were retired once their
functionality was ported into `kode-optimasi/` or superseded by it — still
in git history if needed, just not in the working tree.
