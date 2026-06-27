#pragma once
// ============================================================
// hash.h — md5 / sha1 / crc32 哈希函数
// 纯 C 实现，零外部依赖，零堆分配
// ============================================================

#include <stdint.h>
#include <string.h>
#include "types.h"

// 前向声明
static inline char* str_pool_alloc(int len);

/* ─── MD5 (RFC 1321) ─────────────────────────────────── */

typedef struct {
    uint32_t v[4]; uint64_t len; uint8_t buf[64]; int pos;
} _md5_ctx;

static inline void _md5_init(_md5_ctx *c) {
    c->v[0]=0x67452301; c->v[1]=0xEFCDAB89; c->v[2]=0x98BADCFE; c->v[3]=0x10325476;
    c->len=0; c->pos=0;
}

#define _MD5_F(x,y,z) ((z)^((x)&((y)^(z))))
#define _MD5_G(x,y,z) ((y)^((z)&((x)^(y))))
#define _MD5_H(x,y,z) ((x)^(y)^(z))
#define _MD5_I(x,y,z) ((y)^((x)|~(z)))
#define _MD5_ROTL(v,s) (((v)<<(s))|((v)>>(32-(s))))
#define _MD5_STEP(f,a,b,c,d,x,s,t) do{a+=f(b,c,d)+x+t;a=_MD5_ROTL(a,s);a+=b;}while(0)

static inline void _md5_block(_md5_ctx *c) {
    uint32_t x[16], a=c->v[0], b=c->v[1], cc=c->v[2], d=c->v[3];
    for(int i=0;i<16;i++)x[i]=(uint32_t)c->buf[i*4]|(uint32_t)c->buf[i*4+1]<<8|(uint32_t)c->buf[i*4+2]<<16|(uint32_t)c->buf[i*4+3]<<24;
    _MD5_STEP(_MD5_F,a,b,cc,d,x[0],7,0xD76AA478);_MD5_STEP(_MD5_F,d,a,b,cc,x[1],12,0xE8C7B756);_MD5_STEP(_MD5_F,cc,d,a,b,x[2],17,0x242070DB);_MD5_STEP(_MD5_F,b,cc,d,a,x[3],22,0xC1BDCEEE);
    _MD5_STEP(_MD5_F,a,b,cc,d,x[4],7,0xF57C0FAF);_MD5_STEP(_MD5_F,d,a,b,cc,x[5],12,0x4787C62A);_MD5_STEP(_MD5_F,cc,d,a,b,x[6],17,0xA8304613);_MD5_STEP(_MD5_F,b,cc,d,a,x[7],22,0xFD469501);
    _MD5_STEP(_MD5_F,a,b,cc,d,x[8],7,0x698098D8);_MD5_STEP(_MD5_F,d,a,b,cc,x[9],12,0x8B44F7AF);_MD5_STEP(_MD5_F,cc,d,a,b,x[10],17,0xFFFF5BB1);_MD5_STEP(_MD5_F,b,cc,d,a,x[11],22,0x895CD7BE);
    _MD5_STEP(_MD5_F,a,b,cc,d,x[12],7,0x6B901122);_MD5_STEP(_MD5_F,d,a,b,cc,x[13],12,0xFD987193);_MD5_STEP(_MD5_F,cc,d,a,b,x[14],17,0xA679438E);_MD5_STEP(_MD5_F,b,cc,d,a,x[15],22,0x49B40821);
    _MD5_STEP(_MD5_G,a,b,cc,d,x[1],5,0xF61E2562);_MD5_STEP(_MD5_G,d,a,b,cc,x[6],9,0xC040B340);_MD5_STEP(_MD5_G,cc,d,a,b,x[11],14,0x265E5A51);_MD5_STEP(_MD5_G,b,cc,d,a,x[0],20,0xE9B6C7AA);
    _MD5_STEP(_MD5_G,a,b,cc,d,x[5],5,0xD62F105D);_MD5_STEP(_MD5_G,d,a,b,cc,x[10],9,0x02441453);_MD5_STEP(_MD5_G,cc,d,a,b,x[15],14,0xD8A1E681);_MD5_STEP(_MD5_G,b,cc,d,a,x[4],20,0xE7D3FBC8);
    _MD5_STEP(_MD5_G,a,b,cc,d,x[9],5,0x21E1CDE6);_MD5_STEP(_MD5_G,d,a,b,cc,x[14],9,0xC33707D6);_MD5_STEP(_MD5_G,cc,d,a,b,x[3],14,0xF4D50D87);_MD5_STEP(_MD5_G,b,cc,d,a,x[8],20,0x455A14ED);
    _MD5_STEP(_MD5_G,a,b,cc,d,x[13],5,0xA9E3E905);_MD5_STEP(_MD5_G,d,a,b,cc,x[2],9,0xFCEFA3F8);_MD5_STEP(_MD5_G,cc,d,a,b,x[7],14,0x676F02D9);_MD5_STEP(_MD5_G,b,cc,d,a,x[12],20,0x8D2A4C8A);
    _MD5_STEP(_MD5_H,a,b,cc,d,x[5],4,0xFFFA3942);_MD5_STEP(_MD5_H,d,a,b,cc,x[8],11,0x8771F681);_MD5_STEP(_MD5_H,cc,d,a,b,x[11],16,0x6D9D6122);_MD5_STEP(_MD5_H,b,cc,d,a,x[14],23,0xFDE5380C);
    _MD5_STEP(_MD5_H,a,b,cc,d,x[1],4,0xA4BEEA44);_MD5_STEP(_MD5_H,d,a,b,cc,x[4],11,0x4BDECFA9);_MD5_STEP(_MD5_H,cc,d,a,b,x[7],16,0xF6BB4B60);_MD5_STEP(_MD5_H,b,cc,d,a,x[10],23,0xBEBFBC70);
    _MD5_STEP(_MD5_H,a,b,cc,d,x[13],4,0x289B7EC6);_MD5_STEP(_MD5_H,d,a,b,cc,x[0],11,0xEAA127FA);_MD5_STEP(_MD5_H,cc,d,a,b,x[3],16,0xD4EF3085);_MD5_STEP(_MD5_H,b,cc,d,a,x[6],23,0x04881D05);
    _MD5_STEP(_MD5_H,a,b,cc,d,x[9],4,0xD9D4D039);_MD5_STEP(_MD5_H,d,a,b,cc,x[12],11,0xE6DB99E5);_MD5_STEP(_MD5_H,cc,d,a,b,x[15],16,0x1FA27CF8);_MD5_STEP(_MD5_H,b,cc,d,a,x[2],23,0xC4AC5665);
    _MD5_STEP(_MD5_I,a,b,cc,d,x[0],6,0xF4292244);_MD5_STEP(_MD5_I,d,a,b,cc,x[7],10,0x432AFF97);_MD5_STEP(_MD5_I,cc,d,a,b,x[14],15,0xAB9423A7);_MD5_STEP(_MD5_I,b,cc,d,a,x[5],21,0xFC93A039);
    _MD5_STEP(_MD5_I,a,b,cc,d,x[12],6,0x655B59C3);_MD5_STEP(_MD5_I,d,a,b,cc,x[3],10,0x8F0CCC92);_MD5_STEP(_MD5_I,cc,d,a,b,x[10],15,0xFFEFF47D);_MD5_STEP(_MD5_I,b,cc,d,a,x[1],21,0x85845DD1);
    _MD5_STEP(_MD5_I,a,b,cc,d,x[8],6,0x6FA87E4F);_MD5_STEP(_MD5_I,d,a,b,cc,x[15],10,0xFE2CE6E0);_MD5_STEP(_MD5_I,cc,d,a,b,x[6],15,0xA3014314);_MD5_STEP(_MD5_I,b,cc,d,a,x[13],21,0x4E0811A1);
    _MD5_STEP(_MD5_I,a,b,cc,d,x[4],6,0xF7537E82);_MD5_STEP(_MD5_I,d,a,b,cc,x[11],10,0xBD3AF235);_MD5_STEP(_MD5_I,cc,d,a,b,x[2],15,0x2AD7D2BB);_MD5_STEP(_MD5_I,b,cc,d,a,x[9],21,0xEB86D391);
    c->v[0]+=a; c->v[1]+=b; c->v[2]+=cc; c->v[3]+=d;
}

static inline t_string tphp_fn_md5(t_string s) {
    _md5_ctx c; _md5_init(&c);
    uint8_t *d = (uint8_t*)(STR_PTR(s) ? STR_PTR(s) : "");
    int len = s.length;
    uint64_t blen = (uint64_t)len * 8;
    while (len > 0) { int r = (len < 64 - c.pos) ? len : 64 - c.pos; memcpy(c.buf+c.pos, d, (size_t)r); c.pos += r; d += r; len -= r; if (c.pos == 64) { _md5_block(&c); c.pos = 0; } }
    c.buf[c.pos++] = 0x80; if (c.pos > 56) { while (c.pos < 64) c.buf[c.pos++]=0; _md5_block(&c); c.pos=0; }
    while (c.pos < 56) c.buf[c.pos++] = 0;
    for (int i=0;i<8;i++) c.buf[56+i] = (uint8_t)(blen >> (i*8));
    _md5_block(&c);
    static const char hx[] = "0123456789abcdef";
    char *out = str_pool_alloc(32);
    if (!out) return (t_string){NULL,0};
    for (int i=0;i<4;i++) { uint32_t v=c.v[i]; for(int j=0;j<4;j++){out[i*8+j*2]=hx[(v>>(j*8+4))&0xF];out[i*8+j*2+1]=hx[(v>>(j*8))&0xF];} }
    return (t_string){out,32};
}

/* ─── SHA1 (NIST FIPS 180-4) ──────────────────────────── */

typedef struct { uint32_t v[5]; uint64_t len; uint8_t buf[64]; int pos; } _sha1_ctx;

static inline void _sha1_init(_sha1_ctx *c) {
    c->v[0]=0x67452301;c->v[1]=0xEFCDAB89;c->v[2]=0x98BADCFE;c->v[3]=0x10325476;c->v[4]=0xC3D2E1F0;
    c->len=0;c->pos=0;
}

#define _SHA1_ROTL(v,s) (((v)<<(s))|((v)>>(32-(s))))

static inline void _sha1_block(_sha1_ctx *c) {
    uint32_t w[80], a=c->v[0], b=c->v[1], cc=c->v[2], d=c->v[3], e=c->v[4];
    for(int i=0;i<16;i++)w[i]=(uint32_t)c->buf[i*4]<<24|(uint32_t)c->buf[i*4+1]<<16|(uint32_t)c->buf[i*4+2]<<8|c->buf[i*4+3];
    for(int i=16;i<80;i++)w[i]=_SHA1_ROTL(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
    for(int i=0;i<80;i++){
        uint32_t f,k;
        if(i<20){f=(b&cc)|(~b&d);k=0x5A827999;}
        else if(i<40){f=b^cc^d;k=0x6ED9EBA1;}
        else if(i<60){f=(b&cc)|(b&d)|(cc&d);k=0x8F1BBCDC;}
        else{f=b^cc^d;k=0xCA62C1D6;}
        uint32_t t=_SHA1_ROTL(a,5)+f+e+k+w[i];e=d;d=cc;cc=_SHA1_ROTL(b,30);b=a;a=t;
    }
    c->v[0]+=a;c->v[1]+=b;c->v[2]+=cc;c->v[3]+=d;c->v[4]+=e;
}

static inline t_string tphp_fn_sha1(t_string s) {
    _sha1_ctx c; _sha1_init(&c);
    uint8_t *d = (uint8_t*)(STR_PTR(s) ? STR_PTR(s) : "");
    int len = s.length;
    uint64_t blen = (uint64_t)len * 8;
    while (len > 0) { int r = (len < 64 - c.pos) ? len : 64 - c.pos; memcpy(c.buf+c.pos, d, (size_t)r); c.pos += r; d += r; len -= r; if (c.pos == 64) { _sha1_block(&c); c.pos = 0; } }
    c.buf[c.pos++] = 0x80; if (c.pos > 56) { while (c.pos < 64) c.buf[c.pos++]=0; _sha1_block(&c); c.pos=0; }
    while (c.pos < 56) c.buf[c.pos++] = 0;
    for (int i=0;i<8;i++) c.buf[56+i] = (uint8_t)(blen >> (56-i*8));
    _sha1_block(&c);
    static const char hx[] = "0123456789abcdef";
    char *out = str_pool_alloc(40);
    if (!out) return (t_string){NULL,0};
    for (int i=0;i<5;i++) { uint32_t v=c.v[i]; for(int j=0;j<4;j++){out[i*8+j*2]=hx[(v>>(28-j*8))&0xF];out[i*8+j*2+1]=hx[(v>>(28-j*8+4))&0xF];} }
    return (t_string){out,40};
}

/* ─── CRC32 ────────────────────────────────────────────── */

static uint32_t _crc32_tab[256];
static int _crc32_tab_init = 0;

static inline void _crc32_make_tab() {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) c = (c >> 1) ^ ((c & 1) ? 0xEDB88320UL : 0);
        _crc32_tab[i] = c;
    }
    _crc32_tab_init = 1;
}

static inline t_int tphp_fn_crc32_str(t_string s) {
    if (!_crc32_tab_init) _crc32_make_tab();
    uint32_t crc = 0xFFFFFFFF;
    for (int i = 0; i < s.length; i++) crc = (crc >> 8) ^ _crc32_tab[(crc ^ (unsigned char)STR_PTR(s)[i]) & 0xFF];
    return (t_int)(crc ^ 0xFFFFFFFF);
}
