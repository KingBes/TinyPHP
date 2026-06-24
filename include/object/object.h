#pragma once
// ============================================================
// object.h — COS 风格轻量对象系统
//
//   设计原则：
//   1. 对象头仅 8 字节 (class_id + refcount)，COS Any 风格
//   2. 继承 = struct 嵌套（父类字段在子类头部）
//   3. VTable 直接函数指针调用（AOT 最优）
//   4. 单继承（PHP 语义）
// ============================================================

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

// ── Object header (16 bytes packed) ───────────────────────
//   cls:      direct pointer to class descriptor (no lookup needed)
//   refcount: -1=immortal, 0=stack/auto, >=1=N references
typedef struct _t_object {
    const struct _t_class *cls;
    int32_t                refcount;
} t_object;

// ── Class descriptor (per-class static data) ──────────────
typedef struct _t_class {
    const char            *name;             // debug
    const struct _t_class *parent;           // NULL for root
    uint32_t               instance_size;    // sizeof(struct)
    void                  *dtor;             // void (*dtor)(struct*)
    void                 **vtable;           // [N] function pointers
    uint32_t               vtable_len;       // number of slots
} t_class;

// ── Object lifecycle ──────────────────────────────────────

/** Allocate raw object memory (zeroed), set class pointer and refcount=1 */
static inline void* tp_obj_alloc(const t_class *cls) {
    if (unlikely(cls == NULL || cls->instance_size == 0)) return NULL;
    t_object *obj = (t_object*)calloc(1, (size_t)cls->instance_size);
    if (unlikely(obj == NULL)) return NULL;
    obj->cls = cls;
    obj->refcount = 1;
    return obj;
}

/** Retain */
static inline void* tp_obj_retain(void *obj) {
    if (obj != NULL) {
        t_object *o = (t_object*)obj;
        if (o->refcount > 0) o->refcount++;
    }
    return obj;
}

/** Release (decref → dtor → free). Returns NULL. */
static inline void* tp_obj_release(void *obj) {
    if (obj == NULL) return NULL;
    t_object *o = (t_object*)obj;
    if (o->refcount <= 0) return NULL;
    if (--o->refcount > 0) return NULL;
    const t_class *cls = o->cls;
    if (cls != NULL && cls->dtor != NULL) {
        void (*dtor)(void*) = (void (*)(void*))cls->dtor;
        dtor(obj);
    }
    free(obj);
    return NULL;
}

/** Get class descriptor from object (O(1)) */
static inline const t_class* tp_obj_class(void *obj) {
    return obj ? ((t_object*)obj)->cls : NULL;
}

/** type check with inheritance chain */
static inline int tp_obj_is_a(void *obj, const t_class *cls) {
    if (unlikely(obj == NULL || cls == NULL)) return 0;
    const t_class *oc = ((t_object*)obj)->cls;
    while (oc != NULL) {
        if (oc == cls) return 1;
        oc = oc->parent;
    }
    return 0;
}

/** Upcast via struct nesting offset (compile-time, zero cost) */
#define tp_upcast(obj, child, parent) \
    ((tphp_class_##parent*)((char*)(obj) + offsetof(tphp_class_##child, _parent)))

/** Downcast with runtime check */
#define tp_downcast(obj, child) \
    (tp_obj_is_a((obj), &_class_##child) ? (tphp_class_##child*)(obj) : NULL)
