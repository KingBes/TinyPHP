#pragma once
// ============================================================
// os/times.h — time / date / sleep / usleep
// ============================================================

#include <time.h>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

// === time() — 返回当前 Unix 时间戳 ===
static inline t_int tphp_fn_time(void) {
    return (t_int)time(NULL);
}

// === date() — 格式化时间字符串（PHP 格式，非 strftime） ===
// 手写解析 PHP date 格式字符，零堆分配。
static inline t_string tphp_fn_date(t_string fmt, t_int timestamp) {
    static char out[256];
    time_t t = (time_t)(timestamp > 0 ? timestamp : time(NULL));
    struct tm *tm = localtime(&t);
    if (tm == NULL) return (t_string){NULL, 0};

    char *d = out;
    char *end = out + sizeof(out) - 1;
    int i = 0;
    while (i < fmt.length && d < end) {
        char c = fmt.data[i];
        switch (c) {
            case 'Y': d += snprintf(d, (size_t)(end - d), "%04d", tm->tm_year + 1900); break;
            case 'y': d += snprintf(d, (size_t)(end - d), "%02d", tm->tm_year % 100);  break;
            case 'm': d += snprintf(d, (size_t)(end - d), "%02d", tm->tm_mon + 1);     break;
            case 'n': d += snprintf(d, (size_t)(end - d), "%d",   tm->tm_mon + 1);     break;
            case 'd': d += snprintf(d, (size_t)(end - d), "%02d", tm->tm_mday);        break;
            case 'j': d += snprintf(d, (size_t)(end - d), "%d",   tm->tm_mday);        break;
            case 'H': d += snprintf(d, (size_t)(end - d), "%02d", tm->tm_hour);        break;
            case 'G': d += snprintf(d, (size_t)(end - d), "%d",   tm->tm_hour);        break;
            case 'i': d += snprintf(d, (size_t)(end - d), "%02d", tm->tm_min);         break;
            case 's': d += snprintf(d, (size_t)(end - d), "%02d", tm->tm_sec);         break;
            default:  *d++ = c; break;
        }
        i++;
    }
    return (t_string){.data = out, .length = (int)(d - out)};
}

// === sleep() — 休眠指定秒数 ===
static inline void tphp_fn_sleep(t_int seconds) {
    if (seconds < 0) return;
#ifdef _WIN32
    Sleep((DWORD)(seconds * 1000));
#else
    sleep((unsigned int)seconds);
#endif
}

// === usleep() — 休眠指定微秒数 ===
static inline void tphp_fn_usleep(t_int microseconds) {
    if (microseconds < 0) return;
#ifdef _WIN32
    Sleep((DWORD)(microseconds / 1000));
#else
    usleep((useconds_t)microseconds);
#endif
}

// === hrtime() — 高分辨率时间（纳秒） ===
static inline t_int tphp_fn_hrtime(void) {
#ifdef _WIN32
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (t_int)(now.QuadPart * 1000000000LL / freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (t_int)ts.tv_sec * 1000000000LL + (t_int)ts.tv_nsec;
#endif
}
