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

### 2.1 ✅ 数组预分配容量 + 复用池（已完成）

**做法**：
- 数组字面量 `[1,2,3,4,5]` 创建时预分配 `max(4, len)` 个槽，消除 push 触发 realloc
- 128 槽 LIFO 复用池：`tphp_fn_arr_free` 回收到池，`tphp_fn_arr_create` 优先从池取
- 2× → 1.5× 增长因子（`nc = cap + (cap >> 1)`），减少 25% 内存浪费

**实际收益**：`array_pop` 1.8× 加速；临时数组减少 `malloc/free` 抖动。

### 2.2 ✅ 数组池预热（已完成 — 2025-06）

**做法**：`tphp_rt_init()` 启动时预分配 16 个空数组放入复用池，后续 `[]` 从池 O(1) 获取。

**来源**：参考 PHP 8.5 zend_alloc bin freelist 预热机制。

**实际收益**：数组创建从 12x 慢于 PHP → 4.4x 快于 PHP。

### 2.3 ✅ 小字符串池（已完成）

**做法**：64KB bump allocator（`str_pool_alloc`），≤512 字节字符串零 `malloc`。`str_concat`、`str_dup`、`explode` 片段优先走池。

**实际收益**：`implode`/`explode` 减少 `malloc` 调用。

### 2.4 ✅ ROPE 多片段字符串拼接（已完成 — 2025-06）

**做法**：编译期展平 `"a"."b"."c"` 链为单次 `tphp_rt_str_concat_multi(N, parts)` 调用：
- 第一遍计算总长度 → 1 次 `str_pool_alloc`
- 第二遍逐片 `memcpy`

CodeGenerator 新增 `flattenConcat()` 递归展开 `.` 链，3+ 片段自动触发。

**来源**：参考 PHP 8.5 ROPE opcode (`zend_vm_def.h:3410-3538`)。

**实际收益**：concat-4 从 14x 慢于 PHP → **6.1x 快于 PHP**。

### 2.5 ✅ JSON 编码位图转义 + 批量写入（已完成 — 2025-06）

**做法**：
- 256 位位图 (`json_esc_bits[8]`) O(1) 检测需转义字符
- 连续安全字符批量 `memcpy`（跳过逐字符写入）

**来源**：参考 PHP 8.5 `ext/json/json_encoder.c` bitmap escape + batch safe write。

**实际收益**：json_encode 从 11x 慢于 PHP → **1.2x 慢于 PHP（接近持平）**。

### 2.6 ✅ CodeGenerator 变量作用域提升（已完成 — 2025-06）

**做法**：新增 `$scopeDepth` 计数器追踪 for/while/if/foreach/try 块嵌套深度。嵌套作用域内首次赋值的变量自动提升到 `funcScopeDecls`，注入函数顶部声明。

**实际收益**：消除变量跨块使用的"未声明"编译错误。

### 2.7 ✅ 分支预测优化（已完成）

**做法**：`likely`/`unlikely` 宏标注所有热路径（`arr_item_*`、`arr_index`、`arr_count`、`arr_push`）。TCC/GCC/Clang 均支持 `__builtin_expect`。

### 2.8 ✅ 批量数组构建（已完成）

**做法**：字面量数组 `[1,2,3,4,5]` → `tphp_fn_arr_create(5)` 一次 `calloc`。

**实际收益**：已知长度数组零 `realloc`。

### 2.9 t_var 对象池

**现状**：每次 `push` 新元素创建 `t_var` 栈变量（无 `malloc`），已无此瓶颈。

**状态**：✅ 无需优化（`t_var` 始终栈分配）。

### 2.10 t_string 小字符串优化 (SSO)

**现状**：已有 64KB 字符串池覆盖短字符串。但池满后仍 `malloc`。

**目标**：`t_string` 内置 24 字节缓冲区，短于 24 字节的字符串不堆分配也不占池。

```c
typedef struct {
    char *data;
    int   length;
    char  local[24];  // SSO 缓冲区
    bool  is_local;
} t_string;
```

**预估收益**：字符串拼接密集场景 **2-3x 加速**，零 `malloc`。

### 2.11 Arena / Bump Allocator

**目标**：函数作用域内临时分配使用 arena（bump 指针），函数退出时整块释放。避免逐个 `free`。

**预估收益**：全局 20-30% 提升，彻底消除 `malloc/free` 抖动。

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

| 优化 | 难度 | 收益 | 工作量 | 状态 |
|---|---|---|---|---|
| ROPE 字符串拼接 | ⭐⭐ | 巨大 (100x) | ~50 行 CodeGen + 30 行 runtime | ✅ 已完成 |
| JSON 位图+批量写入 | ⭐ | 大 (10x) | ~40 行 json.h | ✅ 已完成 |
| 数组池预热 | ⭐ | 中 (5x) | ~10 行 runtime.h | ✅ 已完成 |
| 作用域变量提升 | ⭐ | 低 (消除 bug) | ~60 行 CodeGen | ✅ 已完成 |
| Slab/池分配器 | ⭐⭐⭐ | 巨大 (10-20x) | ~200 行 C | ✅ 已完成（数组池 + 字符串池） |
| t_var 对象池 | ⭐ | 中 (5-10x) | ~50 行 C | ✅ 无需（t_var 栈分配） |
| 批量数组构建 | ⭐⭐ | 中 (5-10x) | ~60 行 CodeGen | ✅ 已完成（预分配容量） |
| SSO 字符串 | ⭐⭐ | 中 (2-3x) | ~80 行 C + types.h 改 | 短期 |
| Arena Allocator | ⭐⭐⭐ | 大 (1.5-3x) | ~150 行 C | 短期 |
| for 作用域提升 | ⭐ | 低 (消除 bug) | ~30 行 CodeGen | ✅ 已完成（funcScopeDecls） |

---

## 性能实测

100K 次迭代，TCC 编译，对比 PHP 8.x（纳秒）：

| 场景 | PHP | TinyPHP (优化后) | 比率 |
|---|---|---|---|
| int key 读取 ×100K | 2,982K | 1,111K | **2.7× 快** |
| array_pop ×100K | 4,274K | 2,313K | **1.8× 快** |
| foreach 1K×100K | 1,885,079K | 580,635K | **3.2× 快** |
| 嵌套数组读 ×100K | 3,936K | 1,243K | **3.2× 快** |
| count+for ×100K | 227,532K | 42,192K | **5.4× 快** |
| 数组创建 ×100 | 1,809K | 151K | **12× 快** |
| concat-4 ×500K | 6,829K | 1,115K | **6.1× 快** |
| json_encode ×10K | 2,437K | 2,788K | 1.1× 慢 |

---

## 预期最终性能

优化全部落地后，用 GCC -O2 编译：

| 场景 | 当前 vs PHP | 目标 vs PHP | vs Go | vs Rust |
|---|---|---|---|---|
| 整数循环 | 300-500x | 1000x+ | ~1x | ~0.5x |
| 数组创建 | 4.4x | 10-20x | ~1x | ~0.5x |
| 数组读取 | 4x | 10-20x | ~2x | ~0.8x |
| 字符串拼接 | 6.1x | 10-15x | ~1x | ~0.5x |
