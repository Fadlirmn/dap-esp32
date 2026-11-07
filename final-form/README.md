# final-form

Stream32-ESP-Player's full feature set (SD playlist, likes, Bluetooth A2DP
source, scan/connect, settings, themes) ported onto this project's real
wiring (`old/files/test_input.ino` + `old/files/plan.md`), with the resistive
touchscreen replaced by a rotary encoder + 3 buttons as the primary input.
Touch stays wired in as an optional secondary input (`ENABLE_TOUCH` in
`pins.h`).

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

Bluetooth is **A2DP source** mode: audio streams from the SD card out to a
paired phone/speaker/headset over Bluetooth. There's no local DAC output, so
the PCM5102A I2S wiring some versions of `plan.md` mention is out of scope
here.

## Controls

| Input | Action |
|---|---|
| Rotate encoder | Volume, always |
| PREV / NEXT | Move cursor (hold to repeat) · Player screen: previous/next track |
| Click encoder | Select / open focused item · Player screen: play/pause |
| Double-click encoder (≤400ms) | Player screen: next track |
| Long-press encoder (≥600ms) | Player screen: toggle Like |
| MENU | Back (same per-screen targets as the original touch UI) / open menu from Player |
| Touch (optional) | Same taps as the original Stream32 UI |

Dropped as dead/redundant (already no-ops or duplicates in the original):
bottom-nav "Radio" tab, tapping the header Bluetooth icon as a shortcut,
tapping the "Now Playing" panel on the main menu, the `BT_DEVICES[]` /
`startBluetoothDevice()` single-entry placeholder list (nothing ever called
it), and the Brightness row in Screen settings (hardware constraint, see
above).

## SD card layout

```
/tracks/*.mp3            up to 50 tracks, >= 512 bytes each
/tracks/likes.txt         one path per line, written by toggleLike()/saveLikes()
/tracks/<name>.raw        optional cover art: raw RGB565, 110x110, same
                          basename as its .mp3 (drawCoverArt() falls back to
                          a vector music-note icon if missing)
```

## Libraries to install (Arduino IDE / PlatformIO)

- Adafruit GFX Library, Adafruit ILI9341
- ESP32-A2DP (pschatzmann/ESP32-A2DP)
- minimp3 (single header, place `minimp3.h` next to `audio_decoder.cpp`)
- XPT2046_Touchscreen (only needed if `ENABLE_TOUCH`)
- ESP32 board package (uses `Preferences`, FreeRTOS primitives, `esp_gap_bt_api.h`)

## Known limitations

- Font sizing is approximate: Adafruit_GFX has only one built-in 6x8 font
  scaled by integer `textSize`, so TFT_eSPI's GFX fonts 1/2/4 are mapped to
  sizes 1/2/3 (`gfx_helpers.h:fontToSize`) rather than matched pixel-for-pixel.
- No button equivalent for touch's seek-by-drag on the progress bar; seeking
  is touch-only (`seekToRatio`) until/unless someone wants a seek gesture
  added to the encoder.
- Touch calibration values (`touch.cpp`'s `map()` ranges) are carried over
  unverified from the original hardware — recalibrate once touch is actually
  wired to this board.
- **Not compiled on-device.** No ESP32 toolchain or the libraries above are
  installed in this environment, so this is a careful manual port, not a
  verified build. Build it in the Arduino IDE/PlatformIO on real hardware as
  the next step; `test/test_logic.cpp` only checks the pure-logic pieces that
  don't need the Arduino framework.

## Verifying

```
g++ test/test_logic.cpp -o /tmp/final_form_test && /tmp/final_form_test
```
