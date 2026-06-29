#pragma once
// ============================================================
// rand.h — TinyPHP 随机数（线程安全，CSPRNG 驱动，零全局状态）
//   rand / mt_rand / rand_int / _tphp_random_bytes 统一定义在此
//   builtin.h 的 random_int/random_bytes 依赖此文件
// ============================================================

#include <stdlib.h>
#include <stdint.h>

// ============================================================
// CSPRNG 核心 — 跨平台安全随机字节
//   Windows: rand_s (CRT, TCC/MinGW 均支持)
//   Linux/macOS: /dev/urandom
// ============================================================

static inline int _tphp_random_bytes(unsigned char* buf, size_t n) {
#ifdef _WIN32
    size_t i = 0;
    while (i + 4 <= n) {
        unsigned int v = 0;
        if (rand_s(&v) != 0) return -1;
        buf[i]   = (unsigned char)(v);
        buf[i+1] = (unsigned char)(v >> 8);
        buf[i+2] = (unsigned char)(v >> 16);
        buf[i+3] = (unsigned char)(v >> 24);
        i += 4;
    }
    if (i < n) {
        unsigned int v = 0;
        if (rand_s(&v) != 0) return -1;
        for (; i < n; i++) { buf[i] = (unsigned char)(v); v >>= 8; }
    }
    return 0;
#else
    FILE* f = fopen("/dev/urandom", "rb");
    if (!f) return -1;
    size_t r = fread(buf, 1, n, f);
    fclose(f);
    return (r == n) ? 0 : -1;
#endif
}

// ============================================================
// 通用范围随机整数 — 所有随机函数共用
// ============================================================

static inline t_int tphp_fn_rand_int(t_int min, t_int max) {
    if (min > max) {
        tphp_rt_free_all();
        fputs("\nFatal error: random(): min must be <= max\n\n", stderr);
        exit(1);
    }
    uint64_t range = (uint64_t)(max - min) + 1;
    unsigned char buf[8];
    if (_tphp_random_bytes(buf, 8) != 0) {
        tphp_rt_free_all();
        fputs("\nFatal error: CSPRNG failure\n\n", stderr);
        exit(1);
    }
    uint64_t val = ((uint64_t)buf[0]) | ((uint64_t)buf[1] << 8) | ((uint64_t)buf[2] << 16) |
                   ((uint64_t)buf[3] << 24) | ((uint64_t)buf[4] << 32) | ((uint64_t)buf[5] << 40) |
                   ((uint64_t)buf[6] << 48) | ((uint64_t)buf[7] << 56);
    return min + (t_int)(val % range);
}

// rand / mt_rand — 统一代理到 CSPRNG
static inline t_int tphp_fn_rand(t_int min, t_int max)    { return tphp_fn_rand_int(min, max); }
static inline t_int tphp_fn_mt_rand(t_int min, t_int max) { return tphp_fn_rand_int(min, max); }
