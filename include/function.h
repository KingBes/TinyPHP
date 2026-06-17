#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"

// ============================================================
// tphp_echo — 输出字符串原始字节到 stdout
//   二进制安全，不依赖 \0，不解析格式化占位符
// ============================================================
static inline void tphp_echo(t_string s) {
    if (s.data != NULL && s.length > 0) {
        fwrite(s.data, 1, (size_t)s.length, stdout);
    }
}

// ============================================================
// tphp_safe_malloc — 安全的 malloc，失败则 abort
//   calloc 返回零初始化内存，避免未初始化的指针
// ============================================================
static inline void* tphp_safe_malloc(size_t size) {
    void* p = calloc(1, size);
    if (p == NULL) {
        fputs("FATAL: out of memory\n", stderr);
        abort();
    }
    return p;
}

// ============================================================
// tphp_safe_free — 安全的 free，先检查非空再释放
// ============================================================
static inline void tphp_safe_free(void* p) {
    if (p != NULL) {
        free(p);
    }
}

// ============================================================
// tphp_build_argv — 将 C 的 char** argv 转为 t_array*（万能数组）
//   由 int main() 调用，供 Main.__construct 接收
// ============================================================
static inline t_array* tphp_build_argv(int argc, char **argv) {
    t_array* a = tphp_arr_create();
    if (a == NULL) return NULL;
    for (int i = 0; i < argc; i++) {
        if (argv[i] != NULL) {
            t_string s = {argv[i], (int)strlen(argv[i])};
            tphp_arr_push(a, VAR_STRING(s));
        } else {
            tphp_arr_push(a, VAR_STRING(((t_string){NULL, 0})));
        }
    }
    return a;
}

// ============================================================
// tphp_str_from_int — int → t_string（栈缓冲区，单线程安全）
//   用于 echo 非字符串值时的自动转换
// ============================================================
static inline t_string tphp_str_from_int(t_int v) {
    static char _buf[32];
    int len = snprintf(_buf, sizeof(_buf), "%lld", (long long)v);
    return (t_string){_buf, len > 0 ? len : 0};
}

static inline t_string tphp_str_from_float(t_float v) {
    static char _buf[64];
    int len = snprintf(_buf, sizeof(_buf), "%g", v);
    return (t_string){_buf, len > 0 ? len : 0};
}

static inline t_string tphp_str_from_bool(t_bool v) {
    return v ? STR_LIT("true") : STR_LIT("false");
}

// ============================================================
// tphp_object_free — 统一析构入口
//   refcount 减 1，归零则调用 vtable->dtor 并 free
// ============================================================
static inline void tphp_object_free(t_object *obj) {
    if (obj == NULL) return;
    if (--obj->refcount > 0) return;
    if (obj->vtable != NULL && obj->vtable->dtor != NULL) {
        obj->vtable->dtor(obj);           // 先析构（用户清理内部资源）
    }
    free(obj);                            // 再释放对象自身内存
}
