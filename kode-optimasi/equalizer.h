// 3-band (bass/mid/treble) EQ applied to decoded PCM before it leaves the
// device -- once in audio_decoder.cpp's mp3DecoderTask (Local Player / BT
// Transmitter path) and once in i2s_output.cpp's i2sWriteStereo16 (BT
// Receiver path). No display/audio-library dependency, so it also builds
// unmodified under sim/ for the preset-picker screen.
#pragma once
#include <stdint.h>

typedef enum {
    EQ_FLAT = 0,
    EQ_BASS_BOOST,
    EQ_TREBLE_BOOST,
    EQ_VOCAL,
    EQ_ROCK,
    EQ_CUSTOM, // starts flat (0,0,0) -- otherwise just another editable preset now
    EQ_PRESET_COUNT
} EqPreset;

#define EQ_GAIN_MIN_DB (-12.0f)
#define EQ_GAIN_MAX_DB  (12.0f)

// Per-channel biquad delay elements for the 3-band cascade. Each independent
// output stream (decoder path, receiver path) needs its own instance so one
// stream's filter history never bleeds into the other's.
typedef struct {
    float z1[3];
    float z2[3];
} EqChannelState;

typedef struct {
    EqChannelState l, r;
} EqState;

// Computes EQ_FLAT's coefficients and loads any saved preset gains from SD
// (/eq_presets.cfg, no-op under KODE_OPTIMASI_SIM). Call once at boot, after
// SD.begin().
void equalizerInit(void);
void equalizerSetPreset(EqPreset p);
EqPreset equalizerGetPreset(void);
const char *equalizerPresetName(EqPreset p);

// Every preset's 3 band gains (dB, clamped to +-12) are editable, not just
// EQ_CUSTOM -- Flat/Bass Boost/etc. are just factory starting points.
// Setting a preset's gains while it's the active one re-applies immediately;
// getter fills out_db[3] = {bass, mid, treble}.
void equalizerSetGains(EqPreset p, float bass_db, float mid_db, float treble_db);
void equalizerGetGains(EqPreset p, float out_db[3]);

// Restores `p`'s factory gains (what it shipped with), discarding any edits.
void equalizerResetGains(EqPreset p);

// Writes every preset's current gains to /eq_presets.cfg in one go. Returns
// false if the SD card isn't available (or under KODE_OPTIMASI_SIM).
bool equalizerSaveToSD(void);

// Runs the current preset's bass/mid/treble cascade over one stereo sample
// in place. `state` must be a stable, per-stream EqState (zero-initialized).
void equalizerProcess(EqState *state, int16_t *l, int16_t *r);
