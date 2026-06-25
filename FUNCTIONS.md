# TinyPHP 内置函数参考

> 与 PHP 标准库对照，含实现差异、性能说明。

---

## 快速定位

| 类别 | 函数数 | 跳转 |
|---|---|---|
| 输出/调试 | 2 | [↓](#输出--调试) |
| 类型检测 | 8 | [↓](#类型检测) |
| 类型转换 | 6 | [↓](#类型转换) |
| 数组 | 21 | [↓](#数组) |
| 字符串 | 13 | [↓](#字符串) |
| 数学 | 7 | [↓](#数学) |
| 文件 I/O | 2 | [↓](#文件-io) |
| 时间 | 6 | [↓](#时间) |
| JSON | 2 | [↓](#json) |
| 随机数 | 2 | [↓](#随机数) |
| 异常 | 2 | [↓](#异常) |
| OOP 语法 | 8 | [↓](#oop-语法) |
| C 互操作 | 20+ | [↓](#c-互操作-phpc) |
| **总计** | **70+** | |

---

## 输出 / 调试

| 函数 | C 实现 | 性能 | 差异 |
|---|---|---|---|
| `echo $x` | `tphp_fn_echo(t_string)` → `printf` | ≈PHP | — |
| `var_dump($x)` | type switch → `printf` | ≈PHP | 对象输出 `{}`，不递归属性 |

---

## 类型检测

所有检测编译期静态类型时为**直接字面量**（`true`/`false`），零运行时开销。

| 函数 | AOT 优化 |
|---|---|
| `is_int($x)` | `$x` 类型固定为 `t_int` → 编译期 `true` |
| `is_float/is_string/is_bool/is_array/is_null/is_object` | 同上 |
| `is_callable($x)` | 闭包类型 → `true` |
| `isset($var)` | 非指针类型 → `true`；指针 → `ptr != NULL` |
| `empty($var)` | 按类型分发：int→`==0`，string→`strlen==0`，float/bool 同 |
| `unset($var)` | 对象 → `tp_obj_release`；数组 → `free` |

---

## 类型转换

| 函数 | C 实现 | 性能 |
|---|---|---|
| `intval($x)` | type switch → cast | O(1) |
| `floatval($x)` | type switch → cast | O(1) |
| `strval($x)` | `tphp_rt_str_from_int/float` | O(1) |
| `boolval($x)` | PHP 假值规则 | O(1) |
| `c_int/c_float/c_str` | PHP → C 类型（PHPC 桥接） | O(1) |
| `php_int/php_float/php_str` | C → PHP 类型（PHPC 桥接） | O(1) |

---

## 数组

所有数组函数通过 `include/array.h` 实现。数组为 `t_array*` 指针（128 槽 LIFO 复用池 + 1.5× 增长因子）。

| 函数 | C 实现 | 时间 | 差异 |
|---|---|---|---|
| `count($arr)` | `a->length` | O(1) | — |
| `array_push($arr, $v)` | 追加 entry + grow | O(1) amort. | — |
| `array_pop($arr)` | 取最后一个 entry | O(1) | — |
| `array_shift($arr)` | `memmove` 左移 | O(n) | — |
| `array_unshift($arr, $v)` | `memmove` 右移 + re-key | O(n) | 仅单值 |
| `in_array($v, $arr)` | 线性遍历比较 | O(n) | — |
| `array_search($v, $arr)` | 线性遍历比较 | O(n) | 未找到返回 -1 |
| `array_key_exists($k, $arr)` | 遍历 key | O(n) | — |
| `array_keys($arr)` | 遍历提取 key → 新数组 | O(n) | — |
| `array_values($arr)` | 遍历提取 value → 新数组 | O(n) | — |
| `array_merge($a, $b)` | 逐 entry 复制 | O(n+m) | — |
| `array_sum($arr)` | 遍历累加，int+float 自动提升 | O(n) | — |
| `array_product($arr)` | 遍历累乘，混合提升 | O(n) | — |
| `array_unique($arr)` | ≤16 元素 O(n²)；>16 用开放寻址哈希表 | O(n) 大数组 | — |
| `array_reverse($arr, $pk?)` | 倒序复制 | O(n) | — |
| `array_slice($arr, $off, $len?, $pk?)` | 截取复制 | O(k) | 不支持负 length |
| `array_fill($start, $count, $v)` | `set_int` 填充 | O(n) | — |
| `sort($arr)` | libc `qsort` 原地升序 | O(n log n) | 仅 int/float |
| `rsort($arr)` | `qsort` 降序 | O(n log n) | 仅 int/float |
| `shuffle($arr)` | Fisher-Yates 原地洗牌 | O(n) | — |
| `range($start, $end, $step?)` | 预知长度一次分配 | O(n) | step=0 → error |
| `max($arr)` / `min($arr)` | 遍历比较 | O(n) | 空数组 → error |

---

## 字符串

字符串为 16 字节值类型 `{ char* data; int length; }`。≤512B 通过 64KB bump allocator 分配，零 `malloc`。
**拼接优化**：3+ 片段 `.` 链编译期展平为 ROPE，单次分配替代 N 次 pair-wise。

| 函数 | C 实现 | 性能 | 差异 |
|---|---|---|---|
| `implode($glue, $arr)` | 计算总长 → `str_pool_alloc` → memcpy | O(n) | — |
| `explode($sep, $s)` | 线性查找 + 逐段 `str_pool_alloc` | O(n) | — |
| `strlen($s)` | `s.length` | O(1) | null → 0 |
| `trim($s)` | 首尾遍历 → 无空白时零分配返回原串 | O(n) | 仅 ASCII 空白 |
| `ltrim($s)` / `rtrim($s)` | 遍历 → 无空白时零分配 | O(n) | 同上 |
| `substr($s, $off, $len?)` | 偏移截取 → 全复制时零分配返回原串 | O(1) | 负 offset ✅ / 负 length ✅ |
| `strpos($h, $n)` | `memcmp` 线性查找 | O(n) | 未找到 → -1 |
| `str_contains($h, $n)` | `strpos ≥ 0` | O(n) | — |
| `str_replace($s, $r, $t)` | 两遍扫描 + `str_pool_alloc` | O(n) | 仅字符串参数 |
| `sprintf($fmt, ...)` | `snprintf(NULL,0,...)` 测大小 → `str_pool_alloc` | O(n) | 无长度上限，全 C 格式符 |
| `strtolower($s)` | 逐字符检测 → 无大写时零分配返回原串 | O(n) | 仅 ASCII |
| `strtoupper($s)` | 逐字符检测 → 无小写时零分配返回原串 | O(n) | 仅 ASCII |

---

## 数学

| 函数 | C 实现 | 性能 |
|---|---|---|
| `abs($x)` | `llabs(x)` | O(1) |
| `round($x)` | libc `round()` | O(1) |
| `ceil($x)` | libc `ceil()` | O(1) |
| `floor($x)` | libc `floor()` | O(1) |
| `sqrt($x)` | libc `sqrt(x)`，x<0 → 0 | O(1) |
| `pow($base, $exp)`（`**` 运算符） | `tphp_rt_pow_int` 循环 / `pow()` 浮点 | O(log n) |
| `rand($min, $max)` | libc `rand()` LCG | O(1) |
| `mt_rand($min, $max)` | **MT19937** 真 Mersenne Twister | O(1) |

---

## 文件 I/O

| 函数 | C 实现 | 内存安全 | 差异 |
|---|---|---|---|
| `file_get_contents($path)` | `fopen("rb")` → `fseek/ftell` → `str_pool_alloc` → `fread` → `fclose` | ✅ 配对 | 静态路径，不存在返回空 |
| `file_put_contents($path, $data)` | `fopen("wb")` → `fwrite` → `fclose` | ✅ 配对 | 覆盖写入 |

---

## 时间

| 函数 | C 实现 | 性能 |
|---|---|---|
| `time()` | `time(NULL)` | O(1) |
| `date($fmt)` | `strftime` + 64B 栈缓冲 | O(1) |
| `sleep($s)` | `sleep(s)` | O(s) |
| `usleep($us)` | `usleep(us)` | O(us) |
| `hrtime()` | `QueryPerformanceCounter`(Win) / `clock_gettime`(Unix) | O(1) |
| `microtime()` | 同上，返回 float 秒 | O(1) |

---

## JSON

| 函数 | C 实现 | 内存安全 | 差异 |
|---|---|---|---|
| `json_encode($var)` | 位图转义(256bit O(1)) + 批量安全字符 memcpy → `str_pool_alloc` | ✅ | 对象 → `{}`，无递归保护 |
| `json_decode($s)` | 递归下降解析 → `t_var` | ✅ 无效→error | 无 `assoc` 参数 |

---

## 随机数

| 函数 | 算法 | 周期 | 线程安全 |
|---|---|---|---|
| `rand($min, $max)` | libc LCG | 2^31 | ❌ |
| `mt_rand($min, $max)` | **MT19937** | 2^19937-1 | ❌ |

---

## 异常

| 语法 | C 实现 | 内存安全 |
|---|---|---|
| `try { ... } catch (Exception $e) { ... }` | `setjmp/longjmp` | ✅ `tp_throw` 先 `tphp_rt_free_all()` |
| `finally { ... }` | `TP_FINALLY` 宏 | ✅ 始终执行 |
| `throw new Exception("msg")` | `tp_throw_ex` → 复制消息到 256B 栈缓冲 → `longjmp` | ✅ |
| `throw "string"` | `tp_throw` → `longjmp` | ✅ |

---

## OOP 语法

| 语法 | 实现 | 说明 |
|---|---|---|
| `class B extends A` | COS struct 嵌套 `_parent` | 属性/方法通过父类链解析 |
| `abstract class` | 禁止 `new`，抽象方法无体 | — |
| `interface` | 纯抽象类，不生成 struct | 编译期类型标记 |
| `implements` | 编译期契约 | 不强制检查方法实现 |
| `trait` + `use TraitName` | 方法扁平化 | — |
| `instanceof` | `tp_obj_is_a(obj, &_class_X)` | 遍历类链 |
| `parent::method()` | `&self->_parent` + 父类函数名 | — |
| `__CLASS__` | 编译期字符串常量 | `$this->className` |
| `__METHOD__` | 编译期字符串常量 | `ClassName::methodName` |
| `__destruct` | 作用域结束自动 `tp_obj_release` | 开发者无需手动释放 |

---

## C 互操作 (PHPC)

| 函数 | 方向 | 说明 |
|---|---|---|
| `c_int($x)` | PHP → C | → `int32_t` |
| `c_float($x)` | PHP → C | → `double` |
| `c_str($s)` | PHP → C | → `const char*` |
| `php_int($x)` | C → PHP | → `t_int` |
| `php_float($x)` | C → PHP | → `t_float` |
| `php_str($s)` | C → PHP | → `t_string`（深拷贝） |
| `C->func(args)` | 直接 C 调用 | 无 name mangling |
| `#include "file.h"` | 预处理器 | 生成 `#include` |
| `#flag [CC] [OS] flags` | 预处理器 | 平台+编译器过滤 |
| `#callback type name(params)` | 预处理器 | 声明 C 回调签名 |
| `phpc_arr_int/dbl/str` | PHP→C | 严格类型检查，malloc |
| `phpc_new_arr_int/dbl/str` | C→PHP | 深拷贝 |
| `phpc_obj` | PHP→C | `void*` 对象指针 |
| `phpc_fn($cb)` | 提取 | `cb.func` → `void*` |
| `phpc_env($cb)` | 提取 | `cb.env` → `void*` |
| `phpc_fn_i32/i64/f64` | 类型化 cast | → C 函数指针 |
| `phpc_thunk('name', $fn)` | no-env 回调 | 按 #callback 生成 thunk |
| `phpc_free(ptr)` | 释放 | `free(ptr)` |

---

## 内存安全总览

| 机制 | 说明 |
|---|---|
| 资源追踪链表 | `tphp_rt_register(ptr, type)` 注册 → `error()` 时遍历释放 |
| 64KB 字符串池 | bump allocator，≤512B 零 `malloc`，非池化走 `malloc` |
| 128 槽数组池 | LIFO 复用，1.5× 增长因子 |
| COS refcount | `tp_obj_retain` / `tp_obj_release`，归零 → `__destruct` → `free` |
| scope 自动析构 | `visitMethod` 尾注入 `tp_obj_release(var)` |
| 异常安全 | `tp_throw` 先 `tphp_rt_free_all()` 再 `longjmp` |
| 分支预测 | `likely(x)` 热路径，`unlikely(x)` 错误边界 |
| JSON 安全 | 无效 JSON → `error()` → `tphp_rt_free_all()` → `exit(1)` |
