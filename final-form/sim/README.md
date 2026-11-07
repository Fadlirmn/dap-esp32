# Desktop sim

Renders the real `../theme.cpp`, `../gfx_helpers.cpp`, `../icons.cpp`,
`../screens.cpp`, `../nav.cpp`, `../playlist.cpp`, `../bluetooth.cpp` —
unmodified — to an SDL2 window on your PC, by stubbing only the
hardware-layer headers (`Adafruit_GFX`/`Adafruit_ILI9341`, `SD`,
`Preferences`, `BluetoothA2DPSource`, FreeRTOS). No real SD card, Bluetooth
radio, or MP3 decode — it's a UI/navigation preview, not a functional player.
Fake playlist of 4 tracks is hardcoded in `SD.h`.

## Build

Needs `sdl2` and `freetype2` dev packages (both detected via `pkg-config`).

```
make
./sim
```

## Controls

| Key | Action |
|---|---|
| ↑ / ↓ | Encoder rotate (volume) |
| ← / → | PREV / NEXT (move focus, or skip track in Player) |
| Enter | Encoder click (select / play-pause) |
| Space | Encoder double-click (next track in Player) |
| L | Encoder long-press (toggle like in Player) |
| M | MENU button (open menu / back) |
| B | Toggle fake Bluetooth connected state |
| Esc / Q | Quit |

ponytail: no persistence (`Preferences.h` is a no-op) and no touch path
(`ENABLE_TOUCH` is left undefined so `nav.h`'s touch branch compiles out) —
add a real SD/BT backend here only if the sim needs to do more than preview
the UI.
