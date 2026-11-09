# kode-optimasi — the DAP firmware

LVGL 8.x UI (13 screens, originally ported from a `dap-ui-landscape/` design
mockup — since retired, see git history) for an ESP32 DevKit V1 (WROOM-32,
**no PSRAM**) driving a 320×240 landscape SPI TFT (ILI9341 by default;
ST7789-compatible) via TFT_eSPI. This is the one firmware project in this
repo — the earlier, separate `final-form/` firmware was retired once its
subsystems (listed below) were ported in here.

**Real**: BT A2DP source/sink scan/pair/AVRCP (`bt_backend.*`), MP3 decode +
I2S output (`audio_decoder.*`/`i2s_output.*`), WiFi AP file transfer
(`wifi_transfer.*`), physical rotary encoder + 3-button input (`input.*`,
PREV/NEXT navigate focus, ENC_SW confirms/double-click skips track, rotate =
volume, MENU jumps to the mode grid), XPT2046 touch (`touch.*`).

**Still demo/static data**: the Library/Playlists track catalog (one
hardcoded test file opens at boot as a placeholder, see the `ponytail:`
comment in `kode-optimasi.ino`'s `setup()`) and the header battery
percentage.

## Galeri

Screenshot nyata dari `sim/` (lihat di bawah), bukan mockup — rendernya 100%
lewat LVGL asli + sumber `screen_*.cpp` yang sama persis dengan yang jalan di
ESP32.

| | | |
|---|---|---|
| ![Now Playing](screenshots/01-now-playing.png)<br>1. Now Playing | ![Daftar Lagu](screenshots/02-daftar-lagu.png)<br>2. Daftar Lagu | ![Menu mode](screenshots/03-menu-mode.png)<br>3. Menu mode |
| ![BT Receiver](screenshots/04-bt-receiver.png)<br>4. BT Receiver | ![BT Transmitter](screenshots/05-bt-transmitter.png)<br>5. BT Transmitter | ![Transfer File](screenshots/06-transfer-file.png)<br>6. Transfer File |
| ![Beralih mode](screenshots/07-beralih-mode.png)<br>7. Beralih mode | ![Daftar playlist](screenshots/08-daftar-playlist.png)<br>8. Daftar playlist | ![Isi playlist](screenshots/09-isi-playlist.png)<br>9. Isi playlist |
| ![Volume berubah](screenshots/10-volume.png)<br>10. Volume HUD (overlay) | ![Baterai rendah](screenshots/11-baterai-rendah.png)<br>11. Baterai rendah (overlay) | ![Pilih perangkat (BT Receiver)](screenshots/12-pilih-perangkat-rx.png)<br>12. Pilih Perangkat — RX |
| ![Pilih perangkat (BT Transmitter)](screenshots/13-pilih-perangkat-tx.png)<br>13. Pilih Perangkat — TX | | |

## Board → file map

| Board | Screen | File |
|---|---|---|
| L1 | Now Playing | `screen_now_playing.*` (+ `cassette_anim.*`) |
| L2 | Daftar Lagu | `screen_library.*` |
| L3 | Menu mode | `screen_menu.*` |
| L4 | BT Receiver | `screen_receiver.*` |
| L5 | BT Transmitter | `screen_transmitter.*` |
| L6 | Transfer File | `screen_transfer.*` |
| L7 | Baterai rendah + SD error | `overlay_alert.*` (modal + toast) |
| L8 | Beralih mode | `screen_switch.*` |
| L9 / L10 | Volume berubah / senyap | `overlay_volume.*` (one widget, 2 states) |
| L11 | Daftar playlist | `screen_playlists.*` |
| L12 | Isi playlist | `screen_playlist_detail.*` |
| *(baru)* | Pilih Perangkat (RX/TX) | `screen_bt_devices.*` — not one of the original 12 boards; added so BT Receiver/Transmitter have an actual menu to connect to a device from, instead of just claiming "Terhubung" with nothing chosen. Menu's BT tiles go here first; the connected badge on Receiver/Transmitter taps back to it ("ganti perangkat"). On real hardware this screen's two roles genuinely differ (see "Real Bluetooth" below): TX gets a live scan+connect list, RX gets a passive connection-status readout instead of a fake pick-a-phone list. |

Shared chrome/icons/theme: `ui_common.*`, `ui_icons.*`, `ui_theme.h`.
Lazy screen lifecycle (only one screen's widgets exist at a time, to fit a
no-PSRAM budget across 9+ screens): `screen_manager.*`.
Real classic-BT source/sink: `bt_backend.*` (hardware-only, see below).

## Real Bluetooth (`bt_backend.*`)

Ported from `final-form/bluetooth.*` (A2DP source: scan/connect) and
`final-form/bt_receiver.*` (A2DP sink: connection + AVRCP passthrough), using
the same [ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP) library,
stripped of their UI coupling since `screen_manager.cpp` owns navigation here
instead. Not linked into `sim/` (desktop SDL has no real BT radio) — every
call site in `screen_*.cpp` is `#ifndef KODE_OPTIMASI_SIM`-gated, same pattern
as the dummy-data gating.

- **One role at a time**: source (Transmitter) XOR sink (Receiver), the same
  constraint `final-form/` has. Entering a BT screen starts that role
  on-demand (idempotent — no-op if already that role); no boot-time mode
  pick like `final-form/` does, since the user switches between them live
  from the Menu here.
- **Transmitter (source) — real scan, manual trigger**: entering "Pilih
  Perangkat" for TX does *not* auto-scan (classic-BT inquiry is slow and
  power-hungry) — a "Scan" button runs a 10s discovery window
  (`bt_tx_start_scan()`), with results streaming into the list live (polled
  every 400ms while active) and tapping a result connecting + saving it
  (`Preferences`, survives reboot) for auto-reconnect next time.
- **Receiver (sink) — passive, no scan, no pick-list**: a sink can't "scan"
  for phones — the phone connects to *it* — so "Pilih Perangkat" for RX is
  just a status readout (connected device name, or "Menunggu ponsel
  terhubung...") with a "Lupakan" button to drop the current pairing.
- **AVRCP passthrough is real**: BT Receiver's play/pause/prev/next buttons
  send real transport commands to the connected phone (`bt_rx_play_pause()`
  etc.) — this needs no local decoder since it's just a remote-control
  signal, same as a BT headset's buttons.
- **Audio is real too, now**: the source streams `audio_decoder.cpp`'s
  `get_sound_data()` (SD → minimp3 → PCM) instead of silence; the sink
  forwards received PCM straight to `i2s_output.cpp`'s DAC output. BT
  Transmitter's own on-screen play/pause/prev/next still only affect what's
  displayed (no local playlist to control — see the `ponytail:` placeholder
  in `kode-optimasi.ino`), but the stream itself is real audio.
- Verified by actually compiling this folder for `esp32dev` with the BT
  backend + decoder + WiFi + input all linked in: links cleanly with
  `board_build.partitions = huge_app.csv` in `platformio.ini` (the default
  ESP32 partition table is too small for this combined footprint). **RAM
  35.7% (117 KB), Flash 78.0% (2.45 MB of 3 MB)**.

## Navigation

Real click-driven navigation (not just the demo data):
- Any screen's "MENU" badge (top-right, home icon) → Menu grid.
- A Menu tile → that screen (Daftar Lagu, Playlist, Local Player, Transfer
  File; "Kembali" → Now Playing). The two BT tiles go to **Pilih Perangkat**
  (`screen_bt_devices.cpp`, not one of the original 12 boards) first.
- A device row on Pilih Perangkat → BT Receiver/Transmitter (whichever list
  it was); the "Terhubung" badge there taps back to that same device list
  ("ganti perangkat") — a real round trip, not a dead-end "always connected".
- A Playlists row → that playlist's detail screen; the detail screen's
  back-chevron → back to Playlists.
- A Library row / a Playlist-detail track row → "play this song" (jumps to
  Now Playing) — no row highlight/selection anywhere in these lists by
  design.
- Playlist-detail's "Putar" button → jumps to Now Playing (a real "play this
  playlist" action); "Acak"/"Ulang" are plain buttons with no real
  shuffle/repeat engine behind them, so they intentionally don't fake a
  persistent toggle state — just the same press feedback as everything else.

**Also real, not just decorative anymore**:
- Play/pause (Now Playing, BT Receiver, BT Transmitter) actually toggles —
  icon swaps, and on Now Playing it also pauses the cassette-reel spin
  (`cassette_anim_set_spinning`) and **stops the elapsed-time clock**
  (`demo_cycle.cpp` checks `now_playing_is_playing()` before ticking).
- Prev/next (same three screens) actually cycles through a small demo
  track list — title/subtitle change, and on Now Playing the progress bar
  and elapsed/total time reset and track **that song's own duration**
  (`screen_now_playing.cpp`'s `kDemoTracks[].duration_sec`), not a single
  duration shared by every song.
- Volume (`overlay_volume.cpp`) is a real persistent 0–100 level, not a
  fixed number from a timer — in `sim/`, scroll the mouse wheel to
  raise/lower it.
- Every clickable element gives real press feedback (dims while actually
  held down, via LVGL's native `LV_STATE_PRESSED` — see
  `ui_button_create`/`ui_list_row_create` in `ui_common.cpp`) and reverts the
  instant it's released; nothing stays "selected" just because it was
  clicked once.

**Still demo-only by design**: the battery percentage (`now_playing_set_battery()`
is a no-op stub) and the Library/Playlists track catalog (one hardcoded test
file stands in for a real file picker — see the `ponytail:` comment in
`kode-optimasi.ino`). BT scan/connect/AVRCP/audio, WiFi transfer, and
physical input are all real on hardware now — see "Real Bluetooth" and
"On real hardware" above.

**On real hardware**, the same navigation above works via three input paths,
all registered in `kode-optimasi.ino`'s `setup()`:
- Touch (`touch.*`, XPT2046) — a plain `LV_INDEV_TYPE_POINTER`, identical
  shape to the sim's mouse, so every screen's tap handling above just works
  with zero screen-side changes.
- PREV/NEXT/ENC_SW (`input.*`) — an `LV_INDEV_TYPE_ENCODER` feeding a
  `lv_group` of each screen's buttons/rows (`screen_manager_group()`):
  PREV/NEXT move focus, ENC_SW confirms (clicks the focused widget, same
  event as a tap), ENC_SW double-click skips track on the 3 player screens.
- The rotary encoder's rotation is volume, not focus movement — same
  `overlay_volume_adjust()` as the sim's mouse wheel.

## Build setup

1. **Libraries**: `lvgl` (^8.3), `TFT_eSPI` (^2.5), `ESP32-A2DP`, and
   `XPT2046_Touchscreen`, all installable via the Arduino Library Manager or
   PlatformIO `lib_deps` (already listed in this folder's `platformio.ini`).
2. **`lv_conf.h`** (already in this folder):
   - Arduino IDE: install **lvgl via the Arduino Library Manager** (the
     Arduino-packaged build, not the raw GitHub repo) — it auto-detects
     `lv_conf.h` sitting in the sketch folder, nothing else to do.
   - PlatformIO: the generic `lvgl/lvgl` package does **not** auto-discover a
     project-local `lv_conf.h` — verified by actually building this folder
     with `pio run`. It needs two build flags (already in the `platformio.ini`
     included in this folder): `-D LV_CONF_INCLUDE_SIMPLE=1` and `-I.`
     (so lvgl's `#include "lv_conf.h"` resolves to this folder).
3. **TFT_eSPI pin/driver config** — TFT_eSPI needs these at library-compile
   time, not sketch-time:
   - Arduino IDE: copy `TFT_eSPI_User_Setup.h`'s contents into
     `<Arduino/libraries>/TFT_eSPI/User_Setup.h`.
   - PlatformIO: pass the same `#define`s from that file as `build_flags`
     instead — already done in the included `platformio.ini`.
   - Panel is ILI9341 by default — swap `ILI9341_DRIVER` for `ST7789_DRIVER`
     in `platformio.ini`'s `build_flags` if your panel is ST7789.
4. Flash `kode-optimasi.ino` (Arduino IDE) or `pio run -t upload` (PlatformIO
   — `platformio.ini` sets `src_dir = .`). Touch and physical encoder/button
   input are both wired (see "On real hardware" above).

Verified by actually compiling this folder for `esp32dev`/Arduino against real
`lvgl@8.4.0` + `TFT_eSPI@2.5.43` + `ESP32-A2DP` + `XPT2046_Touchscreen`: links
cleanly — **RAM 35.7% (117 KB), Flash 78.0% (2.45 MB of 3 MB, via
`huge_app.csv`)** — confirms the no-PSRAM RAM budget holds with all 13
screens + 2 overlays + BT source/sink + MP3 decode/I2S + WiFi transfer +
physical input all linked in (only one screen's widgets exist at a time at
runtime, per the lazy screen lifecycle in `screen_manager.cpp`; see "Real
Bluetooth" above for why `huge_app.csv` is needed). Real on-device flashing is
still needed to confirm visuals/timing/actual pairing.

## Desktop preview (`sim/`)

`sim/` renders the real, unmodified screen/overlay sources in an SDL2 window
instead of a TFT panel, against a real
lvgl checkout (lvgl's C source is compiled as C via `gcc`; mixing it into a
C++ `g++` build silently miscompiles it — g++ rejects valid C like implicit
`void*`->`T*` conversions). `cd sim && make sim` (needs network once, to `git
clone` lvgl; needs the `sdl2` dev package via pkg-config) then `./sim`. The
sim registers a real SDL mouse pointer as an LVGL indev, so the navigation
above responds to actual clicks in the window, not a scripted demo.

This caught three real bugs no static review did:
- `lv_obj_set_style_margin_top` doesn't exist in LVGL 8.x (fixed → `pad_top`).
- The original `screen_manager_show()` built the next screen *before*
  freeing the previous one, so for a moment both screens' widgets existed at
  once — on hardware that can blow the no-PSRAM memory pool; in the sim it
  segfaulted inside LVGL's theme code under the deliberately tiny 24 KB pool.
  Fixed by reusing one persistent root screen and `lv_obj_clean()`-ing it
  before rebuilding, so only one screen's widgets ever exist
  (`screen_manager.cpp`).
- There was no input device *and* no click handlers at all at first — every
  screen was pure decoration, auto-cycled by a timer, so nothing was
  navigable. Fixed as described in Navigation above; the old screen-auto-cycle
  timer was removed from `demo_cycle.cpp` since it fought manual clicks.
- Separator text used a UTF-8 "·" (middle dot). LVGL's built-in Montserrat
  fonts only ship the Basic Latin range by default — no Latin-1 Supplement —
  so "·" rendered as a tofu/box glyph. Fixed by switching every occurrence to
  a plain ASCII "-".
- Every clickable widget gave *zero* visual feedback on click at first
  (`ui_card_create`/`ui_button_create` strip the default theme's own press
  style via `remove_style_all`). Fixed with an explicit `LV_STATE_PRESSED`
  style on buttons/rows (see Navigation above) instead of reintroducing the
  theme.

## Known simplifications (see `ponytail:` comments in code for more)

- LVGL's built-in Montserrat font has no bold weight — the spec's "bold"
  title uses the regular 18px face.
- Icons use LVGL's built-in `LV_SYMBOL_*` glyph font, not hand-drawn vector
  art, wherever a matching glyph exists (play/pause/bluetooth/battery/etc.) —
  simpler and still zero bitmap assets. Only the cassette reel (no built-in
  equivalent) is custom vector art (`lv_line` + style-transform rotation).
- `now_playing_set_battery()` is a no-op stub; the header battery is demo-static
  (explicitly out of scope — everything else that looked like a dummy control
  was wired to real, persistent local state instead, see Navigation above).
- The Library/Playlists track catalog is one hardcoded test file, not a real
  SD-card file picker — see the `ponytail:` comment in `kode-optimasi.ino`'s
  `setup()`.
