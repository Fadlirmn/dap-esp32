// No-op NVS stand-in: settings don't persist across sim runs.
// ponytail: real persistence skipped, display-only MVP doesn't need it.
// Add a JSON/file-backed store here if the sim needs to remember settings.
#pragma once
#include "Arduino.h"
#include <cstdint>
#include <cstddef>

class Preferences {
public:
    bool begin(const char *, bool = false) { return true; }
    void end() {}
    void clear() {}

    String getString(const char *, const String &def = String()) { return def; }
    size_t putString(const char *, const String &) { return 0; }
    uint32_t getUInt(const char *, uint32_t def = 0) { return def; }
    size_t putUInt(const char *, uint32_t) { return 0; }
    uint8_t getUChar(const char *, uint8_t def = 0) { return def; }
    size_t putUChar(const char *, uint8_t) { return 0; }
    bool getBool(const char *, bool def = false) { return def; }
    size_t putBool(const char *, bool) { return 0; }
    size_t getBytesLength(const char *) { return 0; }
    size_t getBytes(const char *, void *, size_t) { return 0; }
    size_t putBytes(const char *, const void *, size_t) { return 0; }
};
