# TinyPHP vs Native PHP 8.5 — 性能对比报告

测试环境: Windows x64, PHP 8.5.1 NTS, TinyPHP + TCC AOT  
每次测试迭代 500,000 次（部分测试因耗时调整了迭代次数）

---

## 1. 基础运算 (500K iterations)

| 测试项 | TinyPHP (ns/op) | PHP 8.5 (ns/op) | 倍数 | 胜者 |
|--------|----------------|-----------------|------|------|
| empty-loop | 1.29 | 21.34 | **16.5x** | TinyPHP |
| int-add | 1.63 | 13.62 | **8.4x** | TinyPHP |
| int-mul | 1.78 | 13.56 | **7.6x** | TinyPHP |
| float-div | 1.30 | 13.56 | **10.4x** | TinyPHP |
| int-mod | 1.95 | 13.66 | **7.0x** | TinyPHP |
| float-add | 1.30 | 13.57 | **10.5x** | TinyPHP |
| float-mul | 1.14 | 14.27 | **12.5x** | TinyPHP |

> **结论: 整数/浮点运算 TinyPHP 快 7-17 倍。** AOT 编译为原生机器码，无 VM 调度开销。

## 2. 字符串操作 (500K iterations)

| 测试项 | TinyPHP (ns/op) | PHP 8.5 (ns/op) | 倍数 | 胜者 |
|--------|----------------|-----------------|------|------|
| concat-2 | 76.91 | 13.62 | **0.18x** | PHP |
| concat-4 | 192.86 | 13.66 | **0.07x** | PHP |
| strlen | 4.95 | 16.34 | **3.3x** | TinyPHP |
| trim-no-ws | 13.04 | 21.20 | **1.6x** | TinyPHP |
| trim-ws | 58.70 | 38.96 | 0.66x | PHP |
| strtolower-nc | 107.14 | 41.66 | 0.39x | PHP |
| strtolower-c | 62.75 | 63.98 | 1.02x | 持平 |
| strtoupper | 21.44 | 38.46 | **1.8x** | TinyPHP |
| substr-full | 16.78 | 27.18 | **1.6x** | TinyPHP |
| substr-part | 53.53 | 48.92 | 0.91x | 持平 |
| strpos | 40.36 | 36.51 | 0.90x | 持平 |

> **结论: 字符串拼接 PHP 快 5-14 倍**（PHP 内部用优化后的 zend_string 池），但简单查询（strlen/substr-full）TinyPHP 更快。

## 3. 数组操作

| 测试项 | TinyPHP (ns/op) | PHP 8.5 (ns/op) | 倍数 | 胜者 |
|--------|----------------|-----------------|------|------|
| arr-create | 157.61 | 13.21 | **0.08x** | PHP |
| arr-push-10 | 736.62 | 532.51 | 0.72x | PHP |
| arr-push-100 | 5596.98 | 4812.22 | 0.86x | PHP |
| sort(50) | 1533.18 | 1478.48 | 0.96x | 持平 |
| array_search | 128.38 | 67.14 | 0.52x | PHP |
| in_array | 87.37 | 56.64 | 0.65x | PHP |
| count | 2.69 | 17.21 | **6.4x** | TinyPHP |
| uniq-small(8) | 447.41 | 168.79 | 0.38x | PHP |
| uniq-large(500) | 17963.46 | 14424.36 | 0.80x | PHP |

> **结论: 数组创建 PHP 快 12 倍**（PHP 数组用预分配 slab 分配器）。array_search/in_array PHP 快约 2 倍。count() TinyPHP 快 6.4 倍（直接读字段 vs 哈希表计数）。

## 4. 数学函数 (500K iterations)

| 测试项 | TinyPHP (ns/op) | PHP 8.5 (ns/op) | 倍数 | 胜者 |
|--------|----------------|-----------------|------|------|
| abs | 4.12 | 31.62 | **7.7x** | TinyPHP |
| sqrt | 4.94 | 30.29 | **6.1x** | TinyPHP |
| round | 8.74 | 70.53 | **8.1x** | TinyPHP |
| ceil | 5.53 | 33.90 | **6.1x** | TinyPHP |
| floor | 5.46 | 34.40 | **6.3x** | TinyPHP |

> **结论: 数学函数 TinyPHP 全胜 6-8 倍。** 直接调用 C 标准库，零开销。

## 5. JSON / 文件 (10K/1K iterations)

| 测试项 | TinyPHP (ns/op) | PHP 8.5 (ns/op) | 倍数 | 胜者 |
|--------|----------------|-----------------|------|------|
| json_encode | 2785.40 | 243.68 | **0.09x** | PHP |
| explode | 933.22 | 204.87 | **0.22x** | PHP |
| file-io | 161859.70 | 171164.80 | 1.06x | 持平 |

> **结论: JSON 编码 PHP 快 11 倍**（PHP ext/json 是 C 扩展深度优化）。文件 I/O 持平（OS 瓶颈）。

---

## 总评

| 类别 | TinyPHP | PHP 8.5 |
|------|---------|---------|
| 整数/浮点运算 | ✅ **7-17x 快** | ❌ VM 调度开销 |
| 数学函数 | ✅ **6-8x 快** | ❌ 调用链开销 |
| 字符串拼接 | ❌ 5-14x 慢 | ✅ zend_string 池 |
| 数组操作 | ❌ 1-12x 慢 | ✅ 成熟哈希表 |
| JSON | ❌ 11x 慢 | ✅ C 扩展优化 |
| 文件 I/O | ➖ 持平 | ➖ 持平 |
| 简单查询 (count/strlen) | ✅ **3-6x 快** | ❌ 间接访问 |

### TinyPHP 优势场景
1. **计算密集型**（数学、循环）：AOT 编译为原生机器码，无解释开销
2. **简单属性访问**（count、strlen）：直接读取 C struct 字段
3. **零分配优化路径**（trim-no-ws、strtolower-nc）：返回原字符串指针

### TinyPHP 劣势场景
1. **字符串拼接**：每次拼接都要 malloc + memcpy + free，而 PHP 用引用计数字符串池
2. **数组创建/操作**：每次创建数组都 calloc，而 PHP 有成熟的 slab 分配器
3. **JSON 编码**：自实现 vs PHP ext/json（C 扩展多年优化）

### 核心瓶颈分析
TinyPHP 的主要性能瓶颈在**内存分配**：
- 每个字符串拼接都需要堆分配
- 每个数组创建都需要 malloc/calloc
- 大量小对象分配导致分配器压力

**优化方向**:
1. 字符串池（arena allocator）
2. 数组对象池（已有但容量有限 128）
3. 字符串拼接优化（写时复制 / small-string optimization）
4. 更积极的零分配路径
