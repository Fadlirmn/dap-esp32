# final-form

Stream32-ESP-Player's full feature set (SD playlist, likes, playback) ported
onto this project's real wiring (`old/files/test_input.ino` +
`old/files/plan.md`), with the resistive touchscreen replaced by a rotary
encoder + 3 buttons as the primary input (touch stays wired in as an
optional secondary input, `ENABLE_TOUCH` in `pins.h`), then extended with
four switchable operating modes and a UI redesign matching
`/mnt/seagate/Koding/DAP-Esp32Devkit/dap-ui-landscape/` (320x240 landscape
Material mockup).

## Operating modes

One mode runs per boot, chosen on `STATE_MODE_SELECT` (hold MENU to open it
from anywhere) and persisted in `Preferences`; switching modes shows a
"Beralih mode" screen and does a full `ESP.restart()` — modes don't coexist
live, each owns its radio/I2S hardware exclusively for that boot.

| Mode | What it does |
|---|---|
| **Local Player** | SD card -> wired IEM, over the I2S DAC (`i2s_output.cpp`) |
| **BT Receiver** | Phone audio -> wired IEM, A2DP sink (`bt_receiver.cpp`) over the same I2S output |
| **BT Transmitter** | SD card -> paired Bluetooth headset/speaker, A2DP source (`bluetooth.cpp`) — this is the original single-mode app, now one of four |
| **Transfer File** | ESP32 becomes a WiFi AP + HTTP upload server (`wifi_transfer.cpp`) so you can push files onto the SD card without removing it |

All four are compiled into one firmware image (no build-time feature
flags) — `final-form.ino`'s `setup()`/`loop()` only *initializes* the
subsystems the active mode needs, based on `nav.h`'s `operatingMode`.

## Wiring

| Signal | GPIO |
|---|---|
| TFT CS / DC / RST | 17 / 27 / 33 |
| SPI SCK / MOSI / MISO (shared: TFT+SD+touch) | 18 / 23 / 19 |
| SD CS | 4 |
| Touch CS / IRQ (optional) | 21 / 34 |
| Encoder CLK / DT / SW | 13 / 14 / 16 |
| Buttons PREV / NEXT / MENU | 32 / 36 / 39 |
| TFT backlight | 3V3 (fixed — no PWM pin free, GPIO32 is PREV) |
| I2S DAC BCLK / LRC / DOUT | 25 / 26 / 22 (Local Player + BT Receiver output; confirm against your actual DAC module) |

No battery-sense pin is wired — the header's battery icon shows a hardcoded
dummy value (`layout.h`'s `DUMMY_BATTERY_PCT`) until a sense pin + divider
exist.

## Controls

| Input | Action |
|---|---|
| Rotate encoder | Volume, always (shows a floating auto-hiding overlay) |
| PREV / NEXT | Move cursor (hold to repeat) · Player screen: previous/next track |
| Click encoder | Select / open focused item · Player screen: play/pause |
| Double-click encoder (≤400ms) | Player screen: next track |
| Long-press encoder (≥600ms) | Player screen: toggle Like |
| MENU (short press) | Back (same per-screen targets as the original touch UI) |
| MENU (long press, ≥600ms) | Open the mode-select screen, from anywhere |
| Touch (optional) | Same taps as the original Stream32 UI; not wired for every new screen (see Known limitations) |

## SD card layout

```
/tracks/*.mp3             up to 50 tracks, >= 512 bytes each
/tracks/likes.txt         one path per line, written by toggleLike()/saveLikes()
/tracks/<name>.raw        optional cover art: raw RGB565, same basename as
                          its .mp3. Shown on the Local Player and BT
                          Transmitter "Now Playing" panels; falls back to a
                          vector music-note icon when missing.
/Playlists/*.m3u          named playlists: one file path per line (bare
                          filenames resolve to /tracks/<name>), # = comment
```

## Building

PlatformIO (recommended — `platformio.ini` + `Makefile` already set up):

```
cd final-form
make build                        # pio run
make upload PORT=/dev/ttyUSB0     # pio run -t upload
make monitor PORT=/dev/ttyUSB0    # pio device monitor, 115200 baud
```

Find the port after plugging the board in: `ls /dev/ttyUSB*` or
`/dev/ttyACM*` (Linux), `ls /dev/cu.*` (macOS). `PORT` is optional — PlatformIO
auto-detects it if omitted, but pass it explicitly when more than one
serial device is connected.

`platformio.ini` pulls every dependency via `lib_deps` (including
`pschatzmann/ESP32-A2DP` by git URL — it isn't published on the PlatformIO
registry) and excludes `sim/`/`test/` from the firmware build.
`board_build.partitions = huge_app.csv` because one binary links the A2DP
source *and* sink, WiFi/WebServer, and the full display/touch stack at
once, even though only one mode's subsystems actually start at runtime.

Arduino IDE (manual) instead needs: Adafruit GFX Library, Adafruit
ILI9341, ESP32-A2DP (pschatzmann), XPT2046_Touchscreen (only if
`ENABLE_TOUCH`), and `minimp3.h` (already vendored in this folder, public
domain, from github.com/lieff/minimp3 — no action needed).

Desktop UI sim (no hardware, no ESP32 toolchain — see `sim/README.md`):

```
cd final-form/sim && make && ./sim
```

## Known limitations

- Font sizing is approximate: Adafruit_GFX has only one built-in 6x8 font
  scaled by integer `textSize`, so TFT_eSPI's GFX fonts 1/2/4 are mapped to
  sizes 1/2/3 (`gfx_helpers.h:fontToSize`) rather than matched pixel-for-pixel.
  The hardware font also has no Unicode glyphs — UI text uses "-" where the
  design reference uses "·".
- No button equivalent for touch's seek-by-drag on the progress bar; seeking
  is touch-only (`seekToRatio`) until/unless someone wants a seek gesture
  added to the encoder.
- Touch calibration values (`touch.cpp`'s `map()` ranges) are carried over
  unverified from the original hardware, and several of the newer screens
  (mode-select, BT Receiver, Transfer File, playlists) don't have touch
  hit-tests wired up at all yet — encoder/buttons cover every screen, touch
  is still the optional secondary path and currently a partial one.
- WiFi Transfer mode's AP is `DAP-Transfer` / `dap12345` (hardcoded in
  `wifi_transfer.cpp`) — change it there if you want different credentials.
- I2S DAC pins (25/26/22 above) are a proposed assignment, not yet
  confirmed against real hardware wiring.

## Verifying

Pure-logic unit test (no Arduino framework needed):
```
g++ test/test_logic.cpp -o /tmp/final_form_test && /tmp/final_form_test
```
Full firmware build (needs PlatformIO, see Building above):
```
cd final-form && pio run
```
