#pragma once
// std/type.h — 类型检测/转换 (is_*, intval, gettype, getenv)
//   对应 PHP ext/standard type functions

// tphp_fn_is_* — t_var 类型检测（mixed/union 变量用）
//   静态类型变量在编译期直接生成 true/false，不调用这些函数
// ============================================================
static inline bool tphp_fn_is_int(t_var v)    { return v.type == TYPE_INT; }
static inline bool tphp_fn_is_float(t_var v)  { return v.type == TYPE_FLOAT; }
static inline bool tphp_fn_is_string(t_var v) { return v.type == TYPE_STRING; }
static inline bool tphp_fn_is_bool(t_var v)   { return v.type == TYPE_BOOL; }
static inline bool tphp_fn_is_array(t_var v)  { return v.type == TYPE_ARRAY; }
static inline bool tphp_fn_is_null(t_var v)   { return v.type == TYPE_NULL; }
static inline bool tphp_fn_is_object(t_var v) { return v.type == TYPE_OBJECT; }
static inline bool tphp_fn_is_callable(t_var v) { return v.type == TYPE_CALLBACK; }

/* ============================================================
 * Type conversion functions
/* ============================================================
 * ============================================================ */

static inline t_int tphp_fn_intval(t_var v) {
    if (v.type == TYPE_INT)   return v.value._int;
    if (v.type == TYPE_FLOAT) return (t_int)v.value._float;
    if (v.type == TYPE_BOOL)  return v.value._bool ? 1 : 0;
    if (v.type == TYPE_STRING) return tphp_rt_parse_int(v.value._string);
    return 0;
}

static inline t_float tphp_fn_floatval(t_var v) {
    if (v.type == TYPE_INT)   return (t_float)v.value._int;
    if (v.type == TYPE_FLOAT) return v.value._float;
    if (v.type == TYPE_BOOL)  return v.value._bool ? 1.0 : 0.0;
    if (v.type == TYPE_STRING) return tphp_rt_parse_float(v.value._string);
    return 0.0;
}

static inline t_string tphp_fn_strval(t_var v) {
    if (v.type == TYPE_INT)   return tphp_rt_str_from_int(v.value._int);
    if (v.type == TYPE_FLOAT) return tphp_rt_str_from_float(v.value._float);
    if (v.type == TYPE_BOOL)  return v.value._bool ? STR_LIT("1") : STR_LIT("");
    if (v.type == TYPE_STRING) return tphp_rt_str_dup(v.value._string);
    if (v.type == TYPE_NULL)  return (t_string){.data = NULL, .length = 0, .is_local = false};
    return STR_LIT("");
}

static inline t_bool tphp_fn_boolval(t_var v) {
    if (v.type == TYPE_INT)   return v.value._int != 0;
    if (v.type == TYPE_FLOAT) return v.value._float != 0.0;
    if (v.type == TYPE_BOOL)  return v.value._bool;
    if (v.type == TYPE_STRING) return !tphp_rt_str_is_falsy(v.value._string);
    return false;
}
static inline t_string tphp_fn_gettype(t_var v) {
    static const char *names[] = {
        [TYPE_NULL]     = "NULL",
        [TYPE_INT]      = "int",
        [TYPE_FLOAT]    = "float",
        [TYPE_BOOL]     = "bool",
        [TYPE_STRING]   = "string",
        [TYPE_ARRAY]    = "array",
        [TYPE_OBJECT]   = "object",
        [TYPE_CALLBACK] = "object",
    };
    const char *nm = (v.type <= TYPE_CALLBACK) ? names[v.type] : "unknown";
    return (t_string){(char*)nm, (int)strlen(nm)};
}

// ── getenv / putenv — 环境变量 ───────────────────────────────
static inline t_string tphp_fn_getenv(t_string key) {
    if (key.data == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    static char _env[4096];
    // 临时复制到可写缓冲区（getenv 返回的指针可能不安全）
    char tmp[256];
    int klen = key.length < 255 ? key.length : 255;
    memcpy(tmp, STR_PTR(key), (size_t)klen);
    tmp[klen] = '\0';
    char *val = getenv(tmp);
    if (val == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    int vlen = (int)strlen(val);
    if (vlen > 4095) vlen = 4095;
    memcpy(_env, val, (size_t)vlen);
    _env[vlen] = '\0';
    return (t_string){_env, vlen};
}

static inline void tphp_fn_putenv(t_string key) {
    if (key.data == NULL) return;
    static char _buf[1024];
    int len = key.length < 1023 ? key.length : 1023;
    memcpy(_buf, STR_PTR(key), (size_t)len);
    _buf[len] = '\0';
    putenv(_buf);
}

// ── 第二梯队字符串函数 ──────────────────────────────────────

