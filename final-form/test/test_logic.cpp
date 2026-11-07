// Host-runnable (g++, no Arduino framework) self-check for the pure logic that
// doesn't need Arduino's String/millis/etc. Mirrors nav.cpp/input.cpp/playlist.cpp
// since those files use Arduino types unavailable here -- this re-expresses the
// same algorithms with std::string so a change to the logic without a matching
// test update is the thing to watch for, not a 1:1 #include.
//
// Run: g++ test/test_logic.cpp -o /tmp/final_form_test && /tmp/final_form_test
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

// ---- mirrors nav.cpp's handleEncoderRotate() volume math ----
static int clampVolume(int current, int delta, int step) {
    int v = current + delta * step;
    if (v < 0) return 0;
    if (v > 127) return 127;
    return v;
}

static void test_volume_clamp() {
    assert(clampVolume(70, 1, 4) == 74);
    assert(clampVolume(2, -1, 4) == 0);      // clamps at 0, doesn't go negative
    assert(clampVolume(125, 1, 4) == 127);   // clamps at 127
    assert(clampVolume(70, 0, 4) == 70);
    printf("ok: volume clamp\n");
}

// ---- mirrors input.cpp's pollEncoderSwitch() click/double-click window ----
// Returns "single" or "double" for a release at `nowMs`, given the previous
// pending-click timestamp (or -1 if none pending).
static std::string classifyClick(long pendingClickAtMs, long nowMs, long windowMs) {
    if (pendingClickAtMs >= 0 && nowMs - pendingClickAtMs <= windowMs) return "double";
    return "single";
}

static void test_double_click_window() {
    assert(classifyClick(-1, 1000, 400) == "single");
    assert(classifyClick(1000, 1300, 400) == "double");  // 300ms gap, within window
    assert(classifyClick(1000, 1400, 400) == "double");  // exactly at the boundary
    assert(classifyClick(1000, 1401, 400) == "single");  // 1ms past the boundary
    printf("ok: double-click timing window\n");
}

// ---- mirrors playlist.cpp's trackDisplayName() + sortPlaylist() comparator ----
static std::string trackDisplayName(const std::string &path) {
    std::string name = path;
    size_t slash = name.find_last_of('/');
    if (slash != std::string::npos) name = name.substr(slash + 1);
    for (const char *ext : {".mp3", ".MP3"}) {
        size_t extLen = std::string(ext).size();
        if (name.size() >= extLen && name.compare(name.size() - extLen, extLen, ext) == 0) {
            name.erase(name.size() - extLen);
            break;
        }
    }
    return name;
}

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

static void sortPlaylist(std::vector<std::string> &playlist) {
    std::sort(playlist.begin(), playlist.end(), [](const std::string &a, const std::string &b) {
        return toLower(trackDisplayName(a)) < toLower(trackDisplayName(b));
    });
}

static void test_playlist_sort() {
    assert(trackDisplayName("/tracks/Song.mp3") == "Song");
    assert(trackDisplayName("/tracks/Loud.MP3") == "Loud");
    assert(trackDisplayName("noext") == "noext");

    std::vector<std::string> pl = {"/tracks/Zebra.mp3", "/tracks/apple.mp3", "/tracks/Mango.MP3"};
    sortPlaylist(pl);
    assert(trackDisplayName(pl[0]) == "apple");
    assert(trackDisplayName(pl[1]) == "Mango");
    assert(trackDisplayName(pl[2]) == "Zebra");
    printf("ok: playlist display-name + sort\n");
}

int main() {
    test_volume_clamp();
    test_double_click_window();
    test_playlist_sort();
    printf("all tests passed\n");
    return 0;
}
