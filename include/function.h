#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"

#ifdef _WIN32
#include <windows.h>
#endif

// ============================================================
// tphp_init — 初始化运行时（Windows 下设置控制台 UTF-8）
// ============================================================
static inline void tphp_init(void) {
#ifdef _WIN32
    SetConsoleOutputCP(65001); // CP_UTF8
    SetConsoleCP(65001);
#endif
    (void)0;
}

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
// tphp_parse_int — 从字符串解析整数
//   跳前导空白，识别符号位，提取连续数字
// ============================================================
static inline t_int tphp_parse_int(t_string s) {
    if (s.data == NULL || s.length <= 0) return 0;
    int i = 0;
    while (i < s.length && (s.data[i] == ' ' || s.data[i] == '\t')) i++;
    int sign = 1;
    if (i < s.length && s.data[i] == '-') { sign = -1; i++; }
    else if (i < s.length && s.data[i] == '+') { i++; }
    t_int val = 0;
    while (i < s.length && s.data[i] >= '0' && s.data[i] <= '9') {
        val = val * 10 + (t_int)(s.data[i] - '0');
        i++;
    }
    return val * sign;
}

// ============================================================
// tphp_parse_float — 从字符串解析浮点数（支持科学计数法）
// ============================================================
static inline t_float tphp_parse_float(t_string s) {
    if (s.data == NULL || s.length <= 0) return 0.0;
    char temp[128];
    int len = (s.length < 127) ? s.length : 127;
    memcpy(temp, s.data, (size_t)len);
    temp[len] = '\0';
    return strtod(temp, NULL);
}

// ============================================================
// tphp_str_is_falsy — 字符串是否为 PHP 假值（空串或 "0"）
// ============================================================
static inline t_bool tphp_str_is_falsy(t_string s) {
    if (s.data == NULL || s.length <= 0) return true;
    if (s.length == 1 && s.data[0] == '0') return true;
    return false;
}

// ============================================================
// tphp_err — 运行时致命错误
// ============================================================
static void tphp_err(const char* msg) {
    fprintf(stderr, "Fatal error: %s\n", msg);
    abort();
}

// ============================================================
// tphp_str_concat — 字符串拼接（堆分配，调用方需注意内存）
//   返回新分配的 t_string，data 为 null 终止的堆内存
// ============================================================
static inline t_string tphp_str_concat(t_string a, t_string b) {
    int alen = (a.length > 0 && a.data != NULL) ? a.length : 0;
    int blen = (b.length > 0 && b.data != NULL) ? b.length : 0;
    if (alen == 0 && blen == 0) return (t_string){NULL, 0};
    /* 防止整数溢出 */
    if (alen < 0) alen = 0;
    if (blen < 0) blen = 0;
    if (alen > 0x7FFFFF || blen > 0x7FFFFF) return (t_string){NULL, 0};
    int len = alen + blen;
    if (len <= 0) return (t_string){NULL, 0};
    char* data = (char*)malloc((size_t)len + 1);
    if (data == NULL) return (t_string){NULL, 0};
    int pos = 0;
    if (alen > 0) {
        memcpy(data + pos, a.data, (size_t)alen);
        pos += alen;
    }
    if (blen > 0) {
        memcpy(data + pos, b.data, (size_t)blen);
        pos += blen;
    }
    data[pos] = '\0';
    return (t_string){data, pos};
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

// ============================================================
// tphp_var_dump — PHP var_dump 等价实现
//   根据 t_var 的 type 标签，格式化打印到 stdout
//   支持: int, float, bool, string, null, array, object
// ============================================================
static void tphp_var_dump_indent(int depth) {
    for (int i = 0; i < depth; i++) fputs("  ", stdout);
}

static void tphp_var_dump_rec(t_var v, int depth);

static void tphp_var_dump(t_var v) {
    tphp_var_dump_rec(v, 0);
    fputc('\n', stdout);
}

static void tphp_var_dump_rec(t_var v, int depth) {
    switch (v.type) {
    case TYPE_NULL:
        fputs("NULL", stdout);
        break;
    case TYPE_BOOL:
        fputs(v.value._bool ? "bool(true)" : "bool(false)", stdout);
        break;
    case TYPE_INT:
        fprintf(stdout, "int(%lld)", (long long)v.value._int);
        break;
    case TYPE_FLOAT:
        fprintf(stdout, "float(%g)", v.value._float);
        break;
    case TYPE_STRING: {
        /* 防御: 保护异常长度（负数）和无效指针 */
        int len = (v.value._string.length > 0) ? v.value._string.length : 0;
        fprintf(stdout, "string(%d) \"", len);
        if (len > 0 && v.value._string.data != NULL) {
            fwrite(v.value._string.data, 1, (size_t)len, stdout);
        }
        fputc('"', stdout);
        break;
    }
    case TYPE_CALLBACK:
        fputs("callable", stdout);
        break;
    case TYPE_ARRAY: {
        t_array* a = v.value._array;
        int count = tphp_arr_count(a);
        fprintf(stdout, "array(%d) {\n", count);
        if (a != NULL && a->entries != NULL) {
            for (int i = 0; i < count; i++) {
                t_var* key = a->entries[i].key;
                t_var* val = a->entries[i].value;

                tphp_var_dump_indent(depth + 1);

                /* 安全输出 key */
                if (key == NULL) {
                    fputs("[NULL_KEY]=>\n", stdout);
                } else if (key->type == TYPE_INT) {
                    fprintf(stdout, "[%lld]=>\n", (long long)key->value._int);
                } else if (key->type == TYPE_STRING) {
                    int klen = (key->value._string.length > 0) ? key->value._string.length : 0;
                    fprintf(stdout, "[\"%.*s\"]=>\n",
                        klen,
                        (key->value._string.data != NULL && klen > 0) ? key->value._string.data : "");
                } else {
                    fprintf(stdout, "[?]=>\n");
                }

                tphp_var_dump_indent(depth + 1);

                /* 安全输出 value */
                if (val == NULL) {
                    fputs("NULL", stdout);
                } else {
                    tphp_var_dump_rec(*val, depth + 1);
                }
                fputc('\n', stdout);
            }
        }
        tphp_var_dump_indent(depth);
        fputc('}', stdout);
        break;
    }
    default:
        fputs("unknown", stdout);
        break;
    }
}
