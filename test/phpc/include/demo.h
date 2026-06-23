#pragma once
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// ── 基础函数（已有）─────────────────────────────────────

double calc_distance(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

static char reverse_buf[1024];
const char* reverse_str(const char* input) {
    if (!input) return "";
    int len = (int)strlen(input);
    if (len >= 1023) len = 1023;
    for (int i = 0; i < len; i++) {
        reverse_buf[i] = input[len - 1 - i];
    }
    reverse_buf[len] = '\0';
    return reverse_buf;
}

int64_t factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// ── 数组互操作测试函数 ─────────────────────────────────

// 对 int32_t 数组求和
int64_t sum_ints(const int32_t* data, int len) {
    int64_t s = 0;
    for (int i = 0; i < len; i++) s += data[i];
    return s;
}

// 对 double 数组求和
double sum_dbls(const double* data, int len) {
    double s = 0.0;
    for (int i = 0; i < len; i++) s += data[i];
    return s;
}

// 深拷贝 int 数组（调用方负责 free）
int32_t* copy_ints(const int32_t* data, int len) {
    int32_t* out = (int32_t*)malloc((size_t)len * sizeof(int32_t));
    if (out) memcpy(out, data, (size_t)len * sizeof(int32_t));
    return out;
}

// 对每个元素翻倍（原地修改）
void double_each(int32_t* data, int len) {
    for (int i = 0; i < len; i++) data[i] *= 2;
}

// ── 对象互操作测试：Point ──────────────────────────────

typedef struct {
    double x;
    double y;
} Point;

Point* point_create(double x, double y) {
    Point* p = (Point*)malloc(sizeof(Point));
    if (p) { p->x = x; p->y = y; }
    return p;
}

double point_distance(const Point* p1, const Point* p2) {
    double dx = p2->x - p1->x;
    double dy = p2->y - p1->y;
    return sqrt(dx * dx + dy * dy);
}

double point_norm(const Point* p) {
    return sqrt(p->x * p->x + p->y * p->y);
}

void point_free(Point* p) {
    free(p);
}

// ── 对象互操作 ─────────────────────────────────────────
// 验证 phpc_obj 提取的指针有效（非 NULL）
int obj_valid(void* obj) { return (obj != NULL) ? 1 : 0; }

// 从对象指针读取字段（TinyPHP 对象 = t_object _base + 字段）
// offset: sizeof(t_object)=16 (vtable* + refcount)，x 在其后
double obj_read_x(void* obj, int offset) {
    return *(double*)((char*)obj + offset);
}
double obj_read_y(void* obj, int offset_y) {
    return *(double*)((char*)obj + offset_y);
}

// ── 回调互操作 ──────────────────────────────────────────
// 调用 TinyPHP 闭包：签名 t_int fn(t_int x, void* env)
int64_t apply_closure(t_int (*fn)(t_int x, void* env), void* env, t_int val) {
    return (int64_t)fn(val, env);
}

// 带回调的数组变换：对每个元素调 fn，返回新数组
int32_t* map_ints(const int32_t* src, int len,
                   t_int (*fn)(t_int, void*), void* env) {
    int32_t* out = (int32_t*)malloc((size_t)len * sizeof(int32_t));
    if (!out) return NULL;
    for (int i = 0; i < len; i++) {
        out[i] = (int32_t)fn((t_int)src[i], env);
    }
    return out;
}
