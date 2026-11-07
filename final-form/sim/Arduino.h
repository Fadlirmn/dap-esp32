// Desktop stand-in for the ESP32 Arduino core: just enough of String/Serial/
// millis/FreeRTOS for theme.cpp/gfx_helpers.cpp/icons.cpp/screens.cpp/nav.cpp/
// playlist.cpp/bluetooth.cpp to compile and link unmodified. No real concurrency:
// the "FreeRTOS" calls are no-ops (sim never starts the decoder task).
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cctype>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>

class String {
public:
    String() {}
    String(const char *s) : s_(s ? s : "") {}
    String(const std::string &s) : s_(s) {}
    String(int v) : s_(std::to_string(v)) {}
    String(unsigned int v) : s_(std::to_string(v)) {}
    String(long v) : s_(std::to_string(v)) {}

    size_t length() const { return s_.size(); }
    const char *c_str() const { return s_.c_str(); }
    bool endsWith(const char *suf) const {
        size_t n = strlen(suf);
        return s_.size() >= n && s_.compare(s_.size() - n, n, suf) == 0;
    }
    int lastIndexOf(char c) const {
        auto p = s_.rfind(c);
        return p == std::string::npos ? -1 : (int)p;
    }
    String substring(int start) const {
        if (start < 0) start = 0;
        return (size_t)start >= s_.size() ? String() : String(s_.substr(start));
    }
    String substring(int start, int end) const {
        if (start < 0) start = 0;
        if ((size_t)end > s_.size()) end = (int)s_.size();
        if (end <= start) return String();
        return String(s_.substr(start, end - start));
    }
    void remove(size_t index) { if (index <= s_.size()) s_.erase(index); }
    void toLowerCase() { for (auto &c : s_) c = (char)tolower((unsigned char)c); }
    void trim() {
        size_t a = s_.find_first_not_of(" \t\r\n");
        size_t b = s_.find_last_not_of(" \t\r\n");
        s_ = (a == std::string::npos) ? "" : s_.substr(a, b - a + 1);
    }
    void replace(const char *from, const char *to) {
        std::string f(from), t(to);
        if (f.empty()) return;
        size_t pos = 0;
        while ((pos = s_.find(f, pos)) != std::string::npos) {
            s_.replace(pos, f.size(), t);
            pos += t.size();
        }
    }
    int indexOf(const char *needle) const {
        auto p = s_.find(needle);
        return p == std::string::npos ? -1 : (int)p;
    }
    bool operator==(const String &o) const { return s_ == o.s_; }
    bool operator>(const String &o) const { return s_ > o.s_; }
    String operator+(const char *rhs) const { return String(s_ + rhs); }
    String operator+(const String &rhs) const { return String(s_ + rhs.s_); }
    String &operator+=(const String &rhs) { s_ += rhs.s_; return *this; }

private:
    std::string s_;
};

inline String operator+(const char *lhs, const String &rhs) {
    return String(std::string(lhs) + rhs.c_str());
}

struct SerialClass {
    void begin(unsigned long) {}
    void print(const char *s) { std::fputs(s, stderr); }
    void print(const String &s) { std::fputs(s.c_str(), stderr); }
    void println(const char *s = "") { std::fputs(s, stderr); std::fputc('\n', stderr); }
    void println(const String &s) { println(s.c_str()); }
    void printf(const char *fmt, ...) {
        va_list a; va_start(a, fmt); std::vfprintf(stderr, fmt, a); va_end(a);
    }
};
inline SerialClass Serial;

inline unsigned long millis() {
    using namespace std::chrono;
    static auto start = steady_clock::now();
    return (unsigned long)duration_cast<milliseconds>(steady_clock::now() - start).count();
}
inline void delay(unsigned long ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

template <typename T> T min(T a, T b) { return a < b ? a : b; }
template <typename T> T max(T a, T b) { return a > b ? a : b; }
template <typename T> T constrain(T x, T lo, T hi) { return x < lo ? lo : (x > hi ? hi : x); }

// --- FreeRTOS stand-ins (sim never starts the real decoder task) ---
typedef void *QueueHandle_t;
typedef void *SemaphoreHandle_t;
typedef void *TaskHandle_t;
typedef int BaseType_t;
#define pdPASS 1
#define pdFAIL 0
#define pdTRUE 1
#define pdFALSE 0
#define portMAX_DELAY 0xFFFFFFFFUL
#define portTICK_PERIOD_MS 1
inline unsigned long pdMS_TO_TICKS(unsigned long ms) { return ms; }
inline QueueHandle_t xQueueCreate(int, int) { return (QueueHandle_t)1; }
inline BaseType_t xQueueSend(QueueHandle_t, const void *, unsigned long) { return pdPASS; }
inline BaseType_t xQueueReceive(QueueHandle_t, void *, unsigned long) { return pdFALSE; }
inline void xQueueReset(QueueHandle_t) {}
inline int uxQueueMessagesWaiting(QueueHandle_t) { return 0; }
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return (SemaphoreHandle_t)1; }
inline BaseType_t xSemaphoreTake(SemaphoreHandle_t, unsigned long) { return pdTRUE; }
inline void xSemaphoreGive(SemaphoreHandle_t) {}
typedef void (*TaskFunction_t)(void *);
inline BaseType_t xTaskCreatePinnedToCore(TaskFunction_t, const char *, uint32_t, void *, int, TaskHandle_t *, int) { return pdPASS; }
inline void vTaskDelay(unsigned long) {}
