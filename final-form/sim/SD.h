// Stand-in SD card: no real filesystem. Serves a hardcoded 4-track /tracks
// listing so playlist.cpp's scan logic has something to iterate, and returns
// a fake multi-MB file for any .mp3 open (content is never actually read —
// audio_decoder_stub.cpp never decodes).
#pragma once
#include "Arduino.h"
#include <vector>
#include <cstddef>

#define FILE_READ 0
#define FILE_WRITE 1

class File {
public:
    File() {}
    static File makeDir(std::vector<String> entries) {
        File f; f.isDir_ = true; f.valid_ = true; f.dirEntries_ = entries; return f;
    }
    static File makeTrack(String name, size_t size) {
        File f; f.valid_ = true; f.name_ = name; f.size_ = size; return f;
    }
    static File makeWritable() {
        File f; f.valid_ = true; f.writable_ = true; return f;
    }

    operator bool() const { return valid_; }
    bool isDirectory() const { return isDir_; }
    size_t size() const { return size_; }
    int available() const { return 0; }
    int read(uint8_t *, size_t) { return 0; }
    bool seek(size_t) { return true; }
    void close() { valid_ = false; }
    const char *name() const { return name_.c_str(); }
    void println(const char * = "") {}
    void println(int) {}
    void println(const String &) {}
    String readStringUntil(char) { return String(); }

    File openNextFile() {
        if (!isDir_ || dirIdx_ >= dirEntries_.size()) return File();
        String n = dirEntries_[dirIdx_++];
        return makeTrack(n, 3 * 1024 * 1024);
    }

private:
    bool valid_ = false;
    bool isDir_ = false;
    bool writable_ = false;
    String name_;
    size_t size_ = 0;
    std::vector<String> dirEntries_;
    size_t dirIdx_ = 0;
};

class SDClass {
public:
    bool begin(int = -1) { return true; }
    bool exists(const String &) { return false; }
    bool mkdir(const String &) { return true; }
    bool remove(const String &) { return true; }
    bool rename(const String &, const String &) { return true; }

    File open(const String &pathArg, uint8_t mode = FILE_READ) {
        const char *path = pathArg.c_str();
        std::string p(path);
        if (p == "/tracks") {
            return File::makeDir({"Neon Nights.mp3", "Midnight Drive.mp3",
                                   "Analog Sunrise.mp3", "Open Road.mp3"});
        }
        if (mode == FILE_WRITE) return File::makeWritable();
        if (p.size() >= 4 && p.compare(p.size() - 4, 4, ".mp3") == 0) {
            return File::makeTrack(String(path), 3 * 1024 * 1024);
        }
        return File();
    }
};
inline SDClass SD;
