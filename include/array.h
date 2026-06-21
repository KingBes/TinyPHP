#pragma once
// ============================================================
// PHP 万能数组 API — yyjson 风格 flat memory
//
//   • 所有 entry 在一块 malloc 中（单次分配/释放）
//   • push 2x 扩容 realloc 无需逐 entry malloc
//   • 键: int | string，值: 任意 t_var
//   • 引用计数 + 嵌套数组自动 retain/free
// ============================================================

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "types.h"
#include "runtime.h"

static int tphp_fn_str_hash(t_string s);

// === Lifecycle ===

static inline t_array* tphp_fn_arr_create(int cap) {
    if (cap < 4) cap = 4;
    size_t sz = sizeof(t_array) + (size_t)cap * sizeof(t_arr_entry);
    t_array *a = (t_array*)calloc(1, sz);
    if (a == NULL) return NULL;
    a->refcount = 1;
    a->capacity = cap;
    return a;
}

static inline t_array* tphp_fn_arr_retain(t_array *a) {
    if (a) a->refcount++;
    return a;
}

void tphp_fn_arr_free(t_array *a);  // forward decl, implemented below

// === Internal: grow ===

static inline t_array* tphp_fn_arr_grow(t_array *a, int need) {
    if (a == NULL || need <= a->capacity) return a;
    int nc = a->capacity ? a->capacity * 2 : 4;
    if (nc < need) nc = need;
    size_t sz = sizeof(t_array) + (size_t)nc * sizeof(t_arr_entry);
    t_array *na = (t_array*)realloc(a, sz);
    if (na == NULL) return a;
    na->capacity = nc;
    return na;
}

// === Push (int-key append) ===

static inline t_array* tphp_fn_arr_push(t_array *a, t_var val) {
    if (a == NULL) return NULL;
    a = tphp_fn_arr_grow(a, a->length + 1);
    a->entries[a->length].key.type = TYPE_INT;
    a->entries[a->length].key.value._int = a->length;
    a->entries[a->length].val = val;
    a->length++;
    return a;
}

// === Set by int key ===

static inline t_array* tphp_fn_arr_set_int(t_array *a, t_int key, t_var val) {
    if (a == NULL || key < 0) return a;
    // 线性扫描：如果已存在同键则覆盖
    for (int i = 0; i < a->length; i++) {
        if (a->entries[i].key.type == TYPE_INT && a->entries[i].key.value._int == key) {
            a->entries[i].val = val;
            return a;
        }
    }
    // 追加
    a = tphp_fn_arr_grow(a, a->length + 1);
    a->entries[a->length].key.type = TYPE_INT;
    a->entries[a->length].key.value._int = key;
    a->entries[a->length].val = val;
    a->length++;
    return a;
}

// === Set by str key ===

static inline t_array* tphp_fn_arr_set_str(t_array *a, t_string key, t_var val) {
    if (a == NULL) return a;
    // 线性扫描：覆盖已存在
    for (int i = 0; i < a->length; i++) {
        if (a->entries[i].key.type == TYPE_STRING &&
            tphp_rt_str_eq(a->entries[i].key.value._string, key)) {
            a->entries[i].val = val;
            return a;
        }
    }
    // 追加
    a = tphp_fn_arr_grow(a, a->length + 1);
    a->entries[a->length].key.type = TYPE_STRING;
    a->entries[a->length].key.value._string = tphp_rt_str_dup(key);
    a->entries[a->length].val = val;
    a->length++;
    return a;
}

// === Get by int key (returns t_var*) ===

static inline t_var* tphp_fn_arr_get_int(t_array *a, t_int key) {
    if (a == NULL) return NULL;
    for (int i = 0; i < a->length; i++) {
        if (a->entries[i].key.type == TYPE_INT && a->entries[i].key.value._int == key)
            return &a->entries[i].val;
    }
    return NULL;
}

// === Get by str key (returns t_var*) ===

static inline t_var* tphp_fn_arr_get_str(t_array *a, t_string key) {
    if (a == NULL) return NULL;
    int khash = tphp_fn_str_hash(key);
    for (int i = 0; i < a->length; i++) {
        if (a->entries[i].key.type == TYPE_STRING &&
            tphp_rt_str_eq(a->entries[i].key.value._string, key))
            return &a->entries[i].val;
    }
    return NULL;
}

// === Typed getters for codegen ===

static inline t_int tphp_arr_item_int(t_array *a, int idx) {
    if (a == NULL || idx < 0 || idx >= a->length) return 0;
    t_var *v = &a->entries[idx].val;
    return (v->type == TYPE_INT) ? v->value._int : 0;
}

static inline t_float tphp_arr_item_float(t_array *a, int idx) {
    if (a == NULL || idx < 0 || idx >= a->length) return 0.0;
    t_var *v = &a->entries[idx].val;
    return (v->type == TYPE_FLOAT) ? v->value._float : 0.0;
}

static inline t_string tphp_arr_item_str(t_array *a, int idx) {
    if (a == NULL || idx < 0 || idx >= a->length) return (t_string){NULL, 0};
    t_var *v = &a->entries[idx].val;
    if (v->type == TYPE_STRING) return v->value._string;
    if (v->type == TYPE_INT) {
        static char _b[32];
        int n = snprintf(_b, sizeof(_b), "%lld", (long long)v->value._int);
        return (t_string){.data = _b, .length = n};
    }
    return (t_string){NULL, 0};
}

static inline t_bool tphp_arr_item_bool(t_array *a, int idx) {
    if (a == NULL || idx < 0 || idx >= a->length) return false;
    t_var *v = &a->entries[idx].val;
    return (v->type == TYPE_BOOL) ? v->value._bool : (v->type == TYPE_INT && v->value._int != 0);
}

static inline t_array* tphp_arr_item_array(t_array *a, int idx) {
    if (a == NULL || idx < 0 || idx >= a->length) return NULL;
    t_var *v = &a->entries[idx].val;
    return (v->type == TYPE_ARRAY) ? v->value._array : NULL;
}

static inline void* tphp_arr_item_object(t_array *a, int idx) {
    if (a == NULL || idx < 0 || idx >= a->length) return NULL;
    t_var *v = &a->entries[idx].val;
    return (v->type == TYPE_OBJECT) ? v->value._ptr : NULL;
}

static inline t_callback tphp_arr_item_callback(t_array *a, int idx) {
    if (a == NULL || idx < 0 || idx >= a->length) return (t_callback){NULL, NULL};
    t_var *v = &a->entries[idx].val;
    return (v->type == TYPE_CALLBACK) ? v->value._callback : (t_callback){NULL, NULL};
}

// === Index access (for foreach) ===

static inline t_var* tphp_fn_arr_index(t_array *a, int idx) {
    if (a == NULL || idx < 0 || idx >= a->length) return NULL;
    return &a->entries[idx].val;
}

// === Count ===

static inline int tphp_fn_arr_count(t_array *a) {
    return a ? a->length : 0;
}

// === String-key typed getters ===

static inline t_int tphp_fn_arr_get_str_int(t_array *a, t_string key) {
    t_var *v = tphp_fn_arr_get_str(a, key);
    if (v == NULL) return 0;
    if (v->type == TYPE_INT) return v->value._int;
    if (v->type == TYPE_FLOAT) return (t_int)v->value._float;
    return 0;
}

static inline t_string tphp_fn_arr_get_str_str(t_array *a, t_string key) {
    t_var *v = tphp_fn_arr_get_str(a, key);
    if (v == NULL) return (t_string){NULL, 0};
    if (v->type == TYPE_STRING) return v->value._string;
    if (v->type == TYPE_INT) {
        static char _b[32];
        int n = snprintf(_b, sizeof(_b), "%lld", (long long)v->value._int);
        return (t_string){.data = _b, .length = n};
    }
    return (t_string){NULL, 0};
}

// === Free (single free for all entries + itself) ===

void tphp_fn_arr_free(t_array *a) {
    if (a == NULL) return;
    if (--a->refcount > 0) return;
    for (int i = 0; i < a->length; i++) {
        // 释放 string key（堆分配的）
        if (a->entries[i].key.type == TYPE_STRING)
            tphp_rt_str_free(&a->entries[i].key.value._string);
        // 递归释放嵌套数组
        if (a->entries[i].val.type == TYPE_ARRAY && a->entries[i].val.value._array != NULL)
            tphp_fn_arr_free(a->entries[i].val.value._array);
    }
    free(a);  // 单次 free
}

// === Hash (unchanged) ===

static inline int tphp_fn_str_hash(t_string s) {
    if (s.data == NULL) return 0;
    unsigned int h = 5381;
    for (int i = 0; i < s.length; i++)
        h = ((h << 5) + h) + (unsigned char)s.data[i];
    return (int)h;
}
