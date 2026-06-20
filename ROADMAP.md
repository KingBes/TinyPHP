# TinyPHP 性能优化路线图

> 目标：用"好 C"逼近 Rust/Go 性能。关键原则：**我们管数据结构，编译器管指令级优化。**

---

## 1. 免费午餐 — 换编译器

| 编译器 | 循环性能 | 说明 |
|---|---|---|
| TCC（当前） | 1x 基准 | 零优化，寄存器分配极简 |
| GCC -O2 | **3-8x** | 向量化 + 循环展开 + 内联 |
| Clang/LLVM -O2 | **3-10x** | 同上 + LTO |

**行动**：CI 增加 `-cc gcc` / `-cc clang` 构建产物。本地用户默认 TCC（快编译），生产用 GCC/Clang（快运行）。

---

## 2. 我们能做到的 — 编译器无能为力的

### 2.1 数组 Slab 分配器

**现状**：`t_array` 壳已有对象池（256 槽）。但 `t_entry[]` 数组每 push 一次 `realloc`，5 元素数组 = 3 次分配器调用。

**目标**：预分配 slab，一次分配容纳 N 个小数组的所有 entries。

```c
// 当前：每次 push 触发 realloc
tphp_fn_arr_grow(a, a->length + 1);  // realloc
a->entries[a->length++] = val;        // 赋值

// 优化：Slab 预分配 entry 块
#define SLAB_SIZE 4096
static char slab[SLAB_SIZE];
static int slab_offset = 0;
```

**预估收益**：数组创建场景 **10-20x 加速**（消除 3 次系统调用 → 1 次指针偏移）。

### 2.2 t_var 对象池

**现状**：每次 `push` 新元素 `malloc(sizeof(t_var))`。100K 循环 × 5 元素 = 500K 次 `malloc`。

**目标**：`t_var` 也走对象池（类似 `t_array` 壳的池化）。

**预估收益**：push 操作 **5-10x 加速**。

### 2.3 t_string 小字符串优化 (SSO)

**现状**：`tphp_rt_str_dup` 每次 `malloc`。大量短生命期字符串（缓存在临时变量/echo）重复分配释放。

**目标**：`t_string` 内置 24 字节缓冲区，短于 24 字节的字符串不堆分配。

```c
typedef struct {
    char *data;
    int   length;
    char  local[24];  // SSO 缓冲区
    bool  is_local;
} t_string;
```

**预估收益**：字符串拼接密集场景 **2-3x 加速**，零 `malloc`。

### 2.4 批量数组构建

**现状**：`[1, 2, 3, 4, 5]` 生成 1 次 `create` + 5 次 `push` = 多次 malloc。

**目标**：字面量数组一次性分配完整结构。

```c
// 当前
t_array *a = tphp_fn_arr_create();
tphp_fn_arr_push(a, VAR_INT(1));  // grow+push
...

// 优化
t_array *a = tphp_fn_arr_from(5, values);  // 一次分配
```

**预估收益**：字面量数组创建 **5-10x 加速**。

### 2.5 for 循环作用域提升

**现状**：`for ($i=0; ...)` 声明的 `$i` 出循环体即失效，跨循环需额外声明。

**目标**：CodeGen 自动将 for-init 变量提升到函数作用域（C 兼容）。

**预估收益**：消除编译错误，减少用户心智负担。

---

## 3. 编译器能做的 — 不用管

| 优化 | 谁做 | 说明 |
|---|---|---|
| 寄存器分配 | GCC/Clang | 自动最优 |
| 循环展开 | GCC/Clang | `-funroll-loops` |
| 向量化 (SIMD) | GCC/Clang | `-ftree-vectorize` |
| 函数内联 | GCC/Clang | `-finline-functions` |
| 常量传播/折叠 | GCC/Clang | 编译期计算 |
| 死代码消除 | GCC/Clang | 自动清理 |
| 分支预测优化 | GCC/Clang | `-fprofile-use` |
| LTO 跨文件优化 | GCC/Clang | `-flto` |

---

## 优先级排序

| 优化 | 难度 | 收益 | 工作量 | 建议 |
|---|---|---|---|---|
| Slab 分配器 | ⭐⭐⭐ | 巨大 (10-20x) | ~200 行 C | **最高优先** |
| t_var 对象池 | ⭐ | 中 (5-10x) | ~50 行 C | 立刻做 |
| SSO 字符串 | ⭐⭐ | 中 (2-3x) | ~80 行 C + types.h 改 | 短期 |
| 批量数组构建 | ⭐⭐ | 中 (5-10x) | ~60 行 CodeGen | 短期 |
| for 作用域提升 | ⭐ | 低 (消除 bug) | ~30 行 CodeGen | 顺手修 |

---

## 预期最终性能

优化全部落地后，用 GCC -O2 编译：

| 场景 | 当前 vs PHP | 目标 vs PHP | vs Go | vs Rust |
|---|---|---|---|---|
| 整数循环 | 10-25x | 50-100x | ~1x | ~0.5x |
| 数组创建 | 0.03x | 0.3-0.5x | ~1x | ~0.5x |
| 数组读取 | 4x | 10-20x | ~2x | ~0.8x |
| 字符串拼接 | 2-5x | 10-15x | ~1x | ~0.5x |
