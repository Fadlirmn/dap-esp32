# Porting notes

`final-form/` combines two unrelated sources:

- **Wiring**: `old/files/test_input.ino` (this board's real pins — ILI9341
  via Adafruit_GFX, KY-040 encoder, 3 nav buttons) plus `old/files/plan.md`
  for pins `test_input.ino` didn't cover (SD CS, touch CS/IRQ).
- **UI/features**: `old/Stream32-ESP-Player.ino` (SD playlist, likes,
  Bluetooth A2DP source, scan/connect, settings, themes), originally written
  for different hardware (TFT_eSPI + resistive touchscreen as sole input).

Touch was replaced by the encoder + 3 buttons as primary input; touch is kept
wired in as an optional secondary input (`ENABLE_TOUCH` in `final-form/pins.h`).

Full pin table, control mapping, SD layout, and known limitations are in
`final-form/README.md` — this file only records *why* the port exists.
