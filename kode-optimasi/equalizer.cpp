#include "equalizer.h"
#include <math.h>
#include <string.h>
#ifndef KODE_OPTIMASI_SIM
#include <SD.h>
#define EQ_PRESETS_PATH "/eq_presets.cfg"
#endif

#define SAMPLE_RATE 44100.0f

struct Band {
    float fc;         // center/corner frequency (Hz)
    bool shelf;        // true = shelf (bands 0 and 2), false = peaking (band 1)
    bool high;         // for a shelf band: true = high-shelf, false = low-shelf
};

// band 0 = bass (low shelf @120Hz), band 1 = mid (peaking @1kHz), band 2 = treble (high shelf @6kHz)
static const Band kBands[3] = {
    {120.0f, true, false},
    {1000.0f, false, false},
    {6000.0f, true, true},
};

static const char *kPresetNames[EQ_PRESET_COUNT] = {
    "Flat", "Bass Boost", "Treble Boost", "Vocal", "Rock", "Custom",
};
// What each preset ships with -- equalizerResetGains() restores these.
static const float kFactoryGains[EQ_PRESET_COUNT][3] = {
    {0, 0, 0},    // Flat
    {6, 0, 0},    // Bass Boost
    {0, 0, 6},    // Treble Boost
    {-3, 4, 1},   // Vocal
    {4, -2, 3},   // Rock
    {0, 0, 0},    // Custom
};
// Mutable, loaded from kFactoryGains then overridden by whatever's saved on
// SD -- every preset is user-editable now, not just Custom.
static float s_gains[EQ_PRESET_COUNT][3];

// Normalized (a0 = 1) biquad coefficients, one set per band.
struct Coeffs { float b0, b1, b2, a1, a2; };
static Coeffs s_coeffs[3];
static EqPreset s_preset = EQ_FLAT;

static Coeffs compute_shelf(float fc, float gain_db, bool high) {
    float A = powf(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * (float)M_PI * fc / SAMPLE_RATE;
    float cosw0 = cosf(w0), sinw0 = sinf(w0);
    float alpha = sinw0 * 0.5f * sqrtf(2.0f); // shelf slope S = 1
    float sqrtA2alpha = 2.0f * sqrtf(A) * alpha;

    float b0, b1, b2, a0, a1, a2;
    if (high) {
        b0 =    A * ((A + 1) + (A - 1) * cosw0 + sqrtA2alpha);
        b1 = -2 * A * ((A - 1) + (A + 1) * cosw0);
        b2 =    A * ((A + 1) + (A - 1) * cosw0 - sqrtA2alpha);
        a0 =        (A + 1) - (A - 1) * cosw0 + sqrtA2alpha;
        a1 =    2 * ((A - 1) - (A + 1) * cosw0);
        a2 =        (A + 1) - (A - 1) * cosw0 - sqrtA2alpha;
    } else {
        b0 =    A * ((A + 1) - (A - 1) * cosw0 + sqrtA2alpha);
        b1 =  2 * A * ((A - 1) - (A + 1) * cosw0);
        b2 =    A * ((A + 1) - (A - 1) * cosw0 - sqrtA2alpha);
        a0 =        (A + 1) + (A - 1) * cosw0 + sqrtA2alpha;
        a1 =   -2 * ((A - 1) + (A + 1) * cosw0);
        a2 =        (A + 1) + (A - 1) * cosw0 - sqrtA2alpha;
    }
    return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

static Coeffs compute_peak(float fc, float gain_db) {
    const float Q = 0.9f;
    float A = powf(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * (float)M_PI * fc / SAMPLE_RATE;
    float cosw0 = cosf(w0), sinw0 = sinf(w0);
    float alpha = sinw0 / (2.0f * Q);

    float b0 = 1 + alpha * A;
    float b1 = -2 * cosw0;
    float b2 = 1 - alpha * A;
    float a0 = 1 + alpha / A;
    float a1 = -2 * cosw0;
    float a2 = 1 - alpha / A;
    return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

static void recompute_coeffs(EqPreset p) {
    const float *gain_db = s_gains[p];
    for (int i = 0; i < 3; i++) {
        if (gain_db[i] == 0.0f) {
            s_coeffs[i] = {1, 0, 0, 0, 0}; // unity -- skips the math for a flat band
        } else if (kBands[i].shelf) {
            s_coeffs[i] = compute_shelf(kBands[i].fc, gain_db[i], kBands[i].high);
        } else {
            s_coeffs[i] = compute_peak(kBands[i].fc, gain_db[i]);
        }
    }
}

#ifndef KODE_OPTIMASI_SIM
static bool load_gains_from_sd(void) {
    File f = SD.open(EQ_PRESETS_PATH, FILE_READ);
    if (!f) return false;
    for (int p = 0; p < EQ_PRESET_COUNT; p++) {
        String line = f.readStringUntil('\n');
        float b, m, t;
        if (sscanf(line.c_str(), "%f,%f,%f", &b, &m, &t) != 3) break; // short/corrupt file -- stop, keep factory for the rest
        s_gains[p][0] = b;
        s_gains[p][1] = m;
        s_gains[p][2] = t;
    }
    f.close();
    return true;
}

bool equalizerSaveToSD(void) {
    File f = SD.open(EQ_PRESETS_PATH, FILE_WRITE);
    if (!f) return false;
    for (int p = 0; p < EQ_PRESET_COUNT; p++) {
        f.printf("%.1f,%.1f,%.1f\n", s_gains[p][0], s_gains[p][1], s_gains[p][2]);
    }
    f.close();
    return true;
}
#else
static bool load_gains_from_sd(void) { return false; }
bool equalizerSaveToSD(void) { return false; }
#endif

void equalizerInit(void) {
    memcpy(s_gains, kFactoryGains, sizeof(s_gains));
    load_gains_from_sd(); // overrides whatever's saved -- doesn't change the active preset
    recompute_coeffs(EQ_FLAT);
}

void equalizerSetPreset(EqPreset p) {
    if (p < 0 || p >= EQ_PRESET_COUNT) return;
    s_preset = p;
    recompute_coeffs(p);
}
EqPreset equalizerGetPreset(void) { return s_preset; }
const char *equalizerPresetName(EqPreset p) {
    if (p < 0 || p >= EQ_PRESET_COUNT) return "?";
    return kPresetNames[p];
}

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

void equalizerSetGains(EqPreset p, float bass_db, float mid_db, float treble_db) {
    if (p < 0 || p >= EQ_PRESET_COUNT) return;
    s_gains[p][0] = clampf(bass_db, EQ_GAIN_MIN_DB, EQ_GAIN_MAX_DB);
    s_gains[p][1] = clampf(mid_db, EQ_GAIN_MIN_DB, EQ_GAIN_MAX_DB);
    s_gains[p][2] = clampf(treble_db, EQ_GAIN_MIN_DB, EQ_GAIN_MAX_DB);
    if (s_preset == p) recompute_coeffs(p);
}

void equalizerGetGains(EqPreset p, float out_db[3]) {
    if (p < 0 || p >= EQ_PRESET_COUNT) { out_db[0] = out_db[1] = out_db[2] = 0; return; }
    out_db[0] = s_gains[p][0];
    out_db[1] = s_gains[p][1];
    out_db[2] = s_gains[p][2];
}

void equalizerResetGains(EqPreset p) {
    if (p < 0 || p >= EQ_PRESET_COUNT) return;
    s_gains[p][0] = kFactoryGains[p][0];
    s_gains[p][1] = kFactoryGains[p][1];
    s_gains[p][2] = kFactoryGains[p][2];
    if (s_preset == p) recompute_coeffs(p);
}

// Transposed Direct Form 2, one band in place on `x`.
static inline float run_band(const Coeffs &c, float x, float &z1, float &z2) {
    float y = c.b0 * x + z1;
    z1 = c.b1 * x - c.a1 * y + z2;
    z2 = c.b2 * x - c.a2 * y;
    return y;
}

static inline void run_channel(EqChannelState &st, int16_t *sample) {
    float x = (float)*sample;
    for (int i = 0; i < 3; i++) x = run_band(s_coeffs[i], x, st.z1[i], st.z2[i]);
    if (x > 32767.0f) x = 32767.0f;
    if (x < -32768.0f) x = -32768.0f;
    *sample = (int16_t)x;
}

void equalizerProcess(EqState *state, int16_t *l, int16_t *r) {
    run_channel(state->l, l);
    run_channel(state->r, r);
}
