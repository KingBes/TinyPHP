#pragma once
// std/string.h — 字符串函数
//   对应 PHP ext/standard string functions

/* ============================================================
 * String functions
/* ============================================================
 * ============================================================ */

static inline t_int tphp_fn_strlen(t_string s) {
    return (STR_PTR(s) != NULL && s.length > 0) ? (t_int)s.length : 0;
}

static inline t_string tphp_fn_trim(t_string s) {
    if (STR_PTR(s) == NULL || s.length <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    int start = 0, end = s.length - 1;
    while (start <= end && (unsigned char)STR_PTR(s)[start] <= ' ') start++;
    while (end >= start && (unsigned char)STR_PTR(s)[end] <= ' ') end--;
    int len = end - start + 1;
    if (len <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    if (start == 0 && len == s.length) return s; // zero-alloc shortcut
    char *buf = str_pool_alloc(len);
    if (buf == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    memcpy(buf, STR_PTR(s) + start, (size_t)len);
    buf[len] = '\0';
    return (t_string){buf, len};
}

static inline t_string tphp_fn_ltrim(t_string s) {
    if (STR_PTR(s) == NULL || s.length <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    int start = 0;
    while (start < s.length && (unsigned char)STR_PTR(s)[start] <= ' ') start++;
    int len = s.length - start;
    if (len <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    if (start == 0) return s; // zero-alloc
    char *buf = str_pool_alloc(len);
    if (buf == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    memcpy(buf, STR_PTR(s) + start, (size_t)len);
    buf[len] = '\0';
    return (t_string){buf, len};
}

static inline t_string tphp_fn_rtrim(t_string s) {
    if (STR_PTR(s) == NULL || s.length <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    int end = s.length - 1;
    while (end >= 0 && (unsigned char)STR_PTR(s)[end] <= ' ') end--;
    int len = end + 1;
    if (len <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    if (len == s.length) return s; // zero-alloc
    char *buf = str_pool_alloc(len);
    if (buf == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    memcpy(buf, STR_PTR(s), (size_t)len);
    buf[len] = '\0';
    return (t_string){buf, len};
}

static inline t_string tphp_fn_substr(t_string s, t_int offset, t_int length) {
    if (STR_PTR(s) == NULL || s.length <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    int slen = s.length;
    int start = (int)offset;
    if (start < 0) start = slen + start;
    if (start < 0) start = 0;
    if (start >= slen) return (t_string){.data = NULL, .length = 0, .is_local = false};
    int len;
    if (length < 0) {
        len = slen - start + (int)length;
        if (len < 0) len = 0;
    } else if (length == 0) {
        len = slen - start;
    } else {
        len = (int)length;
        if (start + len > slen) len = slen - start;
    }
    if (len <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    if (start == 0 && len == slen) return s; // zero-alloc full copy
    char *buf = str_pool_alloc(len);
    if (buf == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    memcpy(buf, STR_PTR(s) + start, (size_t)len);
    buf[len] = '\0';
    return (t_string){buf, len};
}

static inline t_int tphp_fn_strpos(t_string haystack, t_string needle) {
    if (STR_PTR(haystack) == NULL || STR_PTR(needle) == NULL) return -1;
    if (needle.length <= 0) return 0;
    if (needle.length > haystack.length) return -1;
    for (int i = 0; i <= haystack.length - needle.length; i++) {
        if (memcmp(STR_PTR(haystack) + i, STR_PTR(needle), (size_t)needle.length) == 0)
            return (t_int)i;
    }
    return -1;
}

static inline t_bool tphp_fn_str_contains(t_string haystack, t_string needle) {
    return tphp_fn_strpos(haystack, needle) >= 0;
}

static inline t_string tphp_fn_sprintf(t_string fmt) {
    // No args — just copy format string
    return tphp_rt_str_dup(fmt);
}

static inline t_string tphp_fn_str_replace(t_string search, t_string replace, t_string subject) {
    if (STR_PTR(subject) == NULL || subject.length <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    if (STR_PTR(search) == NULL || search.length <= 0 || search.length > subject.length)
        return tphp_rt_str_dup(subject);
    // Count occurrences
    int count = 0;
    for (int i = 0; i <= subject.length - search.length; i++) {
        if (memcmp(STR_PTR(subject) + i, STR_PTR(search), (size_t)search.length) == 0) {
            count++; i += search.length - 1;
        }
    }
    if (count == 0) return tphp_rt_str_dup(subject);
    // Calculate new length and build result
    int new_len = subject.length + count * (replace.length - search.length);
    if (new_len <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    char *buf = str_pool_alloc(new_len);
    if (buf == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    int pos = 0, si = 0;
    while (si < subject.length) {
        if (si <= subject.length - search.length &&
            memcmp(STR_PTR(subject) + si, STR_PTR(search), (size_t)search.length) == 0) {
            memcpy(buf + pos, STR_PTR(replace), (size_t)replace.length);
            pos += replace.length;
            si += search.length;
        } else {
            buf[pos++] = STR_PTR(subject)[si++];
        }
    }
    buf[new_len] = '\0';
    return (t_string){buf, new_len};
}

/* ============================================================
 * String (case conversion)
/* ============================================================
 * ============================================================ */

static inline t_string tphp_fn_strtolower(t_string s) {
    if (STR_PTR(s) == NULL || s.length <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    int changed = 0;
    for (int i = 0; i < s.length; i++) {
        unsigned char c = (unsigned char)STR_PTR(s)[i];
        if (c >= 'A' && c <= 'Z') { changed = 1; break; }
    }
    if (!changed) return s; // zero-alloc
    char *buf = str_pool_alloc(s.length);
    if (buf == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    for (int i = 0; i < s.length; i++) {
        unsigned char c = (unsigned char)STR_PTR(s)[i];
        buf[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : (char)c;
    }
    buf[s.length] = '\0';
    return (t_string){buf, s.length};
}

static inline t_string tphp_fn_strtoupper(t_string s) {
    if (STR_PTR(s) == NULL || s.length <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    int changed = 0;
    for (int i = 0; i < s.length; i++) {
        unsigned char c = (unsigned char)STR_PTR(s)[i];
        if (c >= 'a' && c <= 'z') { changed = 1; break; }
    }
    if (!changed) return s; // zero-alloc
    char *buf = str_pool_alloc(s.length);
    if (buf == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    for (int i = 0; i < s.length; i++) {
        unsigned char c = (unsigned char)STR_PTR(s)[i];
        buf[i] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : (char)c;
    }
    buf[s.length] = '\0';
    return (t_string){buf, s.length};
}

static inline t_int tphp_fn_ord(t_string s) {
    if (STR_PTR(s) == NULL || s.length < 1) return 0;
    return (t_int)(unsigned char)STR_PTR(s)[0];
}

static inline t_string tphp_fn_chr(t_int n) {
    static char _chr[2];
    _chr[0] = (char)(n & 0xFF);
    _chr[1] = '\0';
    return (t_string){_chr, 1};
}

// ── str_starts_with / str_ends_with — PHP 8.0+ ──────────────
static inline t_bool tphp_fn_str_starts_with(t_string haystack, t_string needle) {
    if (STR_PTR(haystack) == NULL || STR_PTR(needle) == NULL) return false;
    if (needle.length == 0) return true;
    if (needle.length > haystack.length) return false;
    return memcmp(STR_PTR(haystack), STR_PTR(needle), (size_t)needle.length) == 0;
}

static inline t_bool tphp_fn_str_ends_with(t_string haystack, t_string needle) {
    if (STR_PTR(haystack) == NULL || STR_PTR(needle) == NULL) return false;
    if (needle.length == 0) return true;
    if (needle.length > haystack.length) return false;
    return memcmp(STR_PTR(haystack) + haystack.length - needle.length,
                  STR_PTR(needle), (size_t)needle.length) == 0;
}

// ── is_numeric($str) — 检查是否为数值字符串 ──────────────────
static inline t_bool tphp_fn_is_numeric_str(t_string s) {
    if (STR_PTR(s) == NULL || s.length == 0) return false;
    // 需要 null-terminated 副本给 strto*
    char buf[256];
    int len = s.length < 255 ? s.length : 255;
    memcpy(buf, STR_PTR(s), (size_t)len);
    buf[len] = '\0';
    char *end = NULL;
    strtoll(buf, &end, 10);
    if (end == buf + len) return true;
    strtod(buf, &end);
    if (end == buf + len) return true;
    return false;
}


// ucfirst($s) — 首字符大写，其余不变
static inline t_string tphp_fn_ucfirst(t_string s) {
    if (STR_PTR(s) == NULL || s.length == 0) return s;
    if (STR_PTR(s)[0] < 'a' || STR_PTR(s)[0] > 'z') return s; // 已是零分配
    char *d = str_pool_alloc(s.length);
    if (d == NULL) return s;
    d[0] = (char)(STR_PTR(s)[0] - 32);
    if (s.length > 1) memcpy(d + 1, STR_PTR(s) + 1, (size_t)(s.length - 1));
    d[s.length] = '\0';
    return (t_string){d, s.length};
}

// lcfirst($s) — 首字符小写，其余不变
static inline t_string tphp_fn_lcfirst(t_string s) {
    if (STR_PTR(s) == NULL || s.length == 0) return s;
    if (STR_PTR(s)[0] < 'A' || STR_PTR(s)[0] > 'Z') return s;
    char *d = str_pool_alloc(s.length);
    if (d == NULL) return s;
    d[0] = (char)(STR_PTR(s)[0] + 32);
    if (s.length > 1) memcpy(d + 1, STR_PTR(s) + 1, (size_t)(s.length - 1));
    d[s.length] = '\0';
    return (t_string){d, s.length};
}

// strrev($s) — 反转字符串
static inline t_string tphp_fn_strrev(t_string s) {
    if (STR_PTR(s) == NULL || s.length <= 0) return s;
    char *d = str_pool_alloc(s.length);
    if (d == NULL) return s;
    for (int i = 0; i < s.length; i++) d[i] = STR_PTR(s)[s.length - 1 - i];
    d[s.length] = '\0';
    return (t_string){d, s.length};
}

// str_repeat($s, $n) — 重复字符串
static inline t_string tphp_fn_str_repeat(t_string s, t_int n) {
    if (STR_PTR(s) == NULL || s.length <= 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    if (n < 0) {
        tphp_fn_error((t_string){"str_repeat(): Argument #2 ($times) must be greater than or equal to 0", 71}, "<php>", 0);
        return (t_string){.data = NULL, .length = 0, .is_local = false};
    }
    if (n == 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    int total = s.length * (int)n;
    if (total <= 0 || total > 0x3FFFFF) return (t_string){.data = NULL, .length = 0, .is_local = false};
    char *d = str_pool_alloc(total);
    if (d == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    for (int i = 0; i < (int)n; i++)
        memcpy(d + i * s.length, STR_PTR(s), (size_t)s.length);
    d[total] = '\0';
    return (t_string){d, total};
}

// str_split($s, $chunk?) — 分割字符串为数组，默认 chunk=1
static inline t_array* tphp_fn_str_split(t_string s, t_int chunk) {
    if (chunk < 1) {
        tphp_fn_error((t_string){"str_split(): Argument #2 ($length) must be greater than 0", 56}, "<php>", 0);
        return NULL;
    }
    t_array* out = tphp_fn_arr_create(0);
    if (out == NULL) return NULL;
    tphp_rt_register((void*)out, 1);
    if (STR_PTR(s) == NULL || s.length <= 0) return out;
    int pieces = (s.length + (int)chunk - 1) / (int)chunk;
    for (int i = 0; i < pieces; i++) {
        int start = i * (int)chunk;
        int len = (int)chunk;
        if (start + len > s.length) len = s.length - start;
        char *p = str_pool_alloc(len);
        if (p == NULL) break;
        memcpy(p, STR_PTR(s) + start, (size_t)len);
        p[len] = '\0';
        out = tphp_fn_arr_push(out, VAR_STRING(((t_string){p, len})));
    }
    return out;
}

// str_pad($s, $len, $pad?, $type?) — 填充字符串
// type: 0=RIGHT(默认) / 1=LEFT / 2=BOTH
static inline t_string tphp_fn_str_pad(t_string s, t_int len, t_string pad, t_int type) {
    if (STR_PTR(s) == NULL && pad.data == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    int slen = (STR_PTR(s) != NULL) ? s.length : 0;
    int plen = (STR_PTR(pad) != NULL && pad.length > 0) ? pad.length : 1;
    if (len <= slen) return s; // 零分配返回原串
    char *d = str_pool_alloc(len);
    if (d == NULL) return s;
    int gap = len - slen;
    if (type == 1) { // LEFT
        for (int i = 0; i < gap; i++) d[i] = (STR_PTR(pad) != NULL) ? STR_PTR(pad)[i % plen] : ' ';
        if (slen > 0) memcpy(d + gap, STR_PTR(s), (size_t)slen);
    } else if (type == 2) { // BOTH
        int left = gap / 2;
        for (int i = 0; i < left; i++) d[i] = (STR_PTR(pad) != NULL) ? STR_PTR(pad)[i % plen] : ' ';
        if (slen > 0) memcpy(d + left, STR_PTR(s), (size_t)slen);
        int right = gap - left;
        for (int i = 0; i < right; i++) d[left + slen + i] = (STR_PTR(pad) != NULL) ? STR_PTR(pad)[(left + slen + i) % plen] : ' ';
    } else { // RIGHT (default)
        if (slen > 0) memcpy(d, STR_PTR(s), (size_t)slen);
        for (int i = 0; i < gap; i++) d[slen + i] = (STR_PTR(pad) != NULL) ? STR_PTR(pad)[i % plen] : ' ';
    }
    d[len] = '\0';
    return (t_string){d, len};
}

// substr_count($h, $n) — 统计子串出现次数
static inline t_int tphp_fn_substr_count(t_string haystack, t_string needle) {
    if (STR_PTR(haystack) == NULL || STR_PTR(needle) == NULL) return 0;
    if (needle.length == 0 || needle.length > haystack.length) return 0;
    t_int count = 0;
    for (int i = 0; i <= haystack.length - needle.length; i++) {
        if (memcmp(STR_PTR(haystack) + i, STR_PTR(needle), (size_t)needle.length) == 0) {
            count++;
            i += needle.length - 1;
        }
    }
    return count;
}

// str_shuffle($s) — 随机打乱字符串
static inline t_int tphp_fn_rand_int(t_int min, t_int max);

static inline t_string tphp_fn_str_shuffle(t_string s) {
    if (STR_PTR(s) == NULL || s.length <= 0) return s;
    char *d = str_pool_alloc(s.length);
    if (d == NULL) return s;
    memcpy(d, STR_PTR(s), (size_t)s.length);
    // Fisher-Yates
    for (int i = s.length - 1; i > 0; i--) {
        int j = (int)tphp_fn_rand_int(0, i);
        if (j != i) { char t = d[i]; d[i] = d[j]; d[j] = t; }
    }
    d[s.length] = '\0';
    return (t_string){d, s.length};
}

// addslashes($s) — 转义 ' " \ \0
static inline t_string tphp_fn_addslashes(t_string s) {
    if (STR_PTR(s) == NULL || s.length == 0) return s;
    // Pass 1: 数需要转义的字符
    int extra = 0;
    for (int i = 0; i < s.length; i++) {
        char c = STR_PTR(s)[i];
        if (c == '\'' || c == '"' || c == '\\' || c == '\0') extra++;
    }
    if (extra == 0) return s; // 零分配
    int newlen = s.length + extra;
    char *d = str_pool_alloc(newlen);
    if (d == NULL) return s;
    int pos = 0;
    for (int i = 0; i < s.length; i++) {
        char c = STR_PTR(s)[i];
        if (c == '\'' || c == '"' || c == '\\') d[pos++] = '\\';
        d[pos++] = c;
    }
    d[newlen] = '\0';
    return (t_string){d, newlen};
}

// stripslashes($s) — 反转义
static inline t_string tphp_fn_stripslashes(t_string s) {
    if (STR_PTR(s) == NULL || s.length == 0) return s;
    // Pass 1: 数实际上需要的字符数
    int newlen = 0;
    for (int i = 0; i < s.length; i++) {
        if (STR_PTR(s)[i] == '\\' && i + 1 < s.length) { i++; } // 跳转义符
        newlen++;
    }
    if (newlen == s.length) return s; // 零分配
    char *d = str_pool_alloc(newlen);
    if (d == NULL) return s;
    int pos = 0;
    for (int i = 0; i < s.length; i++) {
        if (STR_PTR(s)[i] == '\\' && i + 1 < s.length) { i++; } // 跳过 \\
        d[pos++] = STR_PTR(s)[i];
    }
    d[newlen] = '\0';
    return (t_string){d, newlen};
}

// bin2hex($s) / hex2bin($s) — 二进制 ↔ 十六进制
static inline t_string tphp_fn_bin2hex(t_string s) {
    if (STR_PTR(s) == NULL || s.length == 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    static const char hexc[] = "0123456789abcdef";
    char *d = str_pool_alloc(s.length * 2);
    if (d == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    for (int i = 0; i < s.length; i++) {
        d[i*2]   = hexc[(unsigned char)STR_PTR(s)[i] >> 4];
        d[i*2+1] = hexc[(unsigned char)STR_PTR(s)[i] & 0xF];
    }
    d[s.length*2] = '\0';
    return (t_string){d, s.length * 2};
}

static inline int _is_hex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static int _hexval(char x); // 前置声明

static inline t_string tphp_fn_hex2bin(t_string s) {
    if (STR_PTR(s) == NULL || s.length == 0) return (t_string){.data = NULL, .length = 0, .is_local = false};
    if (s.length % 2 != 0) {
        tphp_fn_error((t_string){"hex2bin(): Hexadecimal input string must have an even length", 58}, "<php>", 0);
        return (t_string){.data = NULL, .length = 0, .is_local = false};
    }
    // validate characters
    for (int i = 0; i < s.length; i++) {
        if (!_is_hex(STR_PTR(s)[i])) {
            tphp_fn_error((t_string){"hex2bin(): Input string must be hexadecimal string", 50}, "<php>", 0);
            return (t_string){.data = NULL, .length = 0, .is_local = false};
        }
    }
    int outlen = s.length / 2;
    char *d = str_pool_alloc(outlen);
    if (d == NULL) return (t_string){.data = NULL, .length = 0, .is_local = false};
    for (int i = 0; i < outlen; i++) {
        int hi = _hexval(STR_PTR(s)[i*2]), lo = _hexval(STR_PTR(s)[i*2+1]);
        d[i] = (char)((hi << 4) | lo);
    }
    d[outlen] = '\0';
    return (t_string){d, outlen};
}

// urlencode($s) / urldecode($s)
static inline int _is_url_safe(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~';
}

static inline t_string tphp_fn_urlencode(t_string s) {
    if (STR_PTR(s) == NULL || s.length == 0) return s;
    // Pass 1: 计算需要%编码的字符
    int extra = 0;
    for (int i = 0; i < s.length; i++) {
        if (!_is_url_safe(STR_PTR(s)[i])) extra += 2;
    }
    if (extra == 0) return s; // 零分配
    char *d = str_pool_alloc(s.length + extra);
    if (d == NULL) return s;
    static const char hx[] = "0123456789ABCDEF";
    int pos = 0;
    for (int i = 0; i < s.length; i++) {
        unsigned char c = (unsigned char)STR_PTR(s)[i];
        if (_is_url_safe((char)c)) { d[pos++] = (char)c; }
        else { d[pos++] = '%'; d[pos++] = hx[c >> 4]; d[pos++] = hx[c & 0xF]; }
    }
    d[pos] = '\0';
    return (t_string){d, pos};
}

static inline int _hexval(char x) {
    if (x >= '0' && x <= '9') return x - '0';
    if (x >= 'A' && x <= 'F') return x - 'A' + 10;
    if (x >= 'a' && x <= 'f') return x - 'a' + 10;
    return 0;
}

static inline t_string tphp_fn_urldecode(t_string s) {
    if (STR_PTR(s) == NULL || s.length == 0) return s;
    // count transformations
    int extra = 0;
    for (int i = 0; i < s.length; i++) {
        if (STR_PTR(s)[i] == '%' && i + 2 < s.length) { extra++; i += 2; }
        else if (STR_PTR(s)[i] == '+') extra++;
    }
    if (extra == 0) return s; // zero-alloc
    char *d = str_pool_alloc(s.length);
    if (d == NULL) return s;
    int pos = 0;
    for (int i = 0; i < s.length; i++) {
        if (STR_PTR(s)[i] == '%' && i + 2 < s.length) {
            int hi = _hexval(STR_PTR(s)[i+1]), lo = _hexval(STR_PTR(s)[i+2]);
            d[pos++] = (char)((hi << 4) | lo);
            i += 2;
        } else if (STR_PTR(s)[i] == '+') {
            d[pos++] = ' ';
        } else {
            d[pos++] = STR_PTR(s)[i];
        }
    }
    d[pos] = '\0';
    return (t_string){d, pos};
}

// ── 第三梯队 ────────────────────────────────────────────────

// parse_str($s) — 解析 query string → 数组 (写入全局作用域风格的简单版)
// 仅返回 key=val 对构成的数组。不支持嵌套键 (a[b]=c)。
static inline t_array* tphp_fn_parse_str(t_string s) {
    t_array *out = tphp_fn_arr_create(8);
    if (out == NULL) return NULL;
    tphp_rt_register((void*)out, 1);
    if (STR_PTR(s) == NULL || s.length == 0) return out;
    int start = 0;
    for (int i = 0; i <= s.length; i++) {
        if (i == s.length || STR_PTR(s)[i] == '&') {
            int seglen = i - start;
            if (seglen > 0) {
                char seg[256]; int sl = seglen < 255 ? seglen : 255;
                memcpy(seg, STR_PTR(s) + start, (size_t)sl); seg[sl] = '\0';
                // 解码 %XX
                char dk[256]; int dp = 0;
                for (int j = 0; j < sl; j++) {
                    if (seg[j] == '%' && j+2 < sl) { int hi=_hexval(seg[j+1]),lo=_hexval(seg[j+2]); seg[j]=(char)((hi<<4)|lo); memmove(seg+j+1,seg+j+3,(size_t)(sl-j-2)); sl-=2; }
                    if (seg[j] == '+') seg[j] = ' ';
                }
                // 找 =
                int eq = -1;
                for (int j = 0; j < sl; j++) if (seg[j] == '=') { eq = j; break; }
                t_string key, val;
                if (eq >= 0) { key = (t_string){seg, eq}; val = (t_string){seg+eq+1, sl-eq-1}; }
                else { key = (t_string){seg, sl}; val = (t_string){.data = NULL, .length = 0, .is_local = false}; }
                out = tphp_fn_arr_set_str(out, key, VAR_STRING(val));
            }
            start = i + 1;
        }
    }
    return out;
}

// parse_url($u) — 解析 URL → 关联数组 (scheme,host,port,path,query)
static inline t_array* tphp_fn_parse_url(t_string u) {
    t_array *out = tphp_fn_arr_create(8);
    if (out == NULL) return NULL;
    tphp_rt_register((void*)out, 1);
    if (u.data == NULL || u.length == 0) return out;

    int pos = 0, len = u.length;

    // scheme://
    int sch = -1;
    for (int i = 0; i < len-2; i++) { if (STR_PTR(u)[i]==':' && STR_PTR(u)[i+1]=='/' && STR_PTR(u)[i+2]=='/') { sch=i; break; } }
    if (sch > 0) {
        t_string _sc = {STR_PTR(u), sch};
        out = tphp_fn_arr_set_str(out, (t_string){"scheme",6}, VAR_STRING(_sc));
        pos = sch + 3;
    }

    // host[:port][/path][?query]
    int host_end = -1, port_n = -1, path_s = -1, q_s = -1;
    for (int i = pos; i < len; i++) {
        if (STR_PTR(u)[i] == ':') { if (port_n < 0) { if (host_end < 0) host_end = i; port_n = i; } }
        else if (STR_PTR(u)[i] == '/') { if (path_s < 0) { if (host_end < 0) host_end = i; path_s = i; } }
        else if (STR_PTR(u)[i] == '?') { if (q_s < 0) { if (host_end < 0) host_end = i; if (path_s < 0) path_s = i; q_s = i; } }
    }
    if (host_end < 0) host_end = len;
    if (host_end > pos) {
        t_string _h = {STR_PTR(u)+pos, host_end-pos};
        out = tphp_fn_arr_set_str(out, (t_string){"host",4}, VAR_STRING(_h));
    }

    // port: from host_end+1 to next / or ?
    if (port_n >= 0) {
        int pe = (path_s >= 0) ? path_s : ((q_s >= 0) ? q_s : len);
        if (pe > port_n + 1) {
            t_string ps = {STR_PTR(u)+port_n+1, pe-port_n-1};
            out = tphp_fn_arr_set_str(out, (t_string){"port",4}, VAR_STRING(ps));
        }
    }

    if (path_s >= 0 && path_s < len) {
        int pe = (q_s >= 0 && q_s < len) ? q_s : len;
        if (pe > path_s) {
            t_string _pa = {STR_PTR(u)+path_s, pe-path_s};
            out = tphp_fn_arr_set_str(out, (t_string){"path",4}, VAR_STRING(_pa));
        }
    }

    if (q_s >= 0 && q_s < len-1) {
        t_string _q = {STR_PTR(u)+q_s+1, len-q_s-1};
        out = tphp_fn_arr_set_str(out, (t_string){"query",5}, VAR_STRING(_q));
    }

    return out;
}

// strtr($s, $from, $to?) — 字符/字符串翻译
static inline t_string tphp_fn_strtr2(t_string s, t_string from, t_string to) {
    if (STR_PTR(s) == NULL || from.data == NULL) return s;
    // 预建翻译表 (仅 ASCII 0-127)
    char map[128]; for (int i = 0; i < 128; i++) map[i] = (char)i;
    int flen = from.length < to.length ? from.length : to.length;
    for (int i = 0; i < flen; i++) map[(unsigned char)STR_PTR(from)[i]] = to.data[i];

    char *d = str_pool_alloc(s.length);
    if (d == NULL) return s;
    for (int i = 0; i < s.length; i++) d[i] = (unsigned char)STR_PTR(s)[i] < 128 ? map[(unsigned char)STR_PTR(s)[i]] : STR_PTR(s)[i];
    d[s.length] = '\0';
    return (t_string){d, s.length};
}

