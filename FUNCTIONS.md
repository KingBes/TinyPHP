# TinyPHP 内置函数参考

> 与 PHP 标准库的对照及实现差异说明。

---

## 输出 / 调试

### `echo` — 输出字符串

```
echo "hello";
echo "a", "b", "c";        // 多参数逗号分隔
```

| | PHP | TinyPHP |
|---|---|---|
| 多参数 | ✅ | ✅ |
| 表达式 | ✅ | ✅ |
| 无返回值 | ✅ | ✅ |

**差异**：无。直接 `fwrite` 输出。

---

### `var_dump` — 递归调试输出

```
var_dump($a);
```

| | PHP | TinyPHP |
|---|---|---|
| 多参数 | ✅ | ❌（仅单参数） |
| 递归打印嵌套数组/对象 | ✅ | ✅ |
| 输出类型+值 | ✅ | ✅ |

**差异**：仅支持单参数。格式为 `type(value)` 而非 PHP 的冗长格式。

---

## 数组

### `count` — 数组元素个数

```
count($arr);
```

| | PHP | TinyPHP |
|---|---|---|
| 递归计数 `COUNT_RECURSIVE` | ✅ | ❌ |
| null/非数组 | 警告 | 未定义行为 |

**差异**：仅支持数组，无递归模式。返回 `int`。

---

### `array_push($arr, $val)` — 尾部追加

```
array_push($arr, 42);
```

| | PHP | TinyPHP |
|---|---|---|
| 单参数追加 | ✅ | ✅ |
| 返回值（新长度） | ✅ | ✅ |
| 多值追加 | ✅ | ❌（仅单值） |

**差异**：仅支持一次 push 单个值。返回新数组长度。

---

### `array_pop($arr)` — 尾部弹出

```
$val = array_pop($arr);
```

| | PHP | TinyPHP |
|---|---|---|
| 空数组返回 null | ✅ | ✅ |
| 返回弹出值 | ✅ | ✅（t_var） |
| 自动缩减长度 | ✅ | ✅ |

**差异**：返回值是 `t_var`（带类型标签），通过 `var_dump` 可正确显示类型。

---

### `in_array($needle, $haystack)` — 值是否存在

```
in_array(42, $arr);
```

| | PHP | TinyPHP |
|---|---|---|
| 严格模式 `===` | ✅ (第三个参数) | ❌ |
| int/string/bool/null 比较 | ✅ | ✅ |
| O(n) 线性扫描 | ✅ | ✅ |

**差异**：不支持严格模式。仅比较 int/string/bool/null 类型。

---

### `array_key_exists($key, $arr)` — 键是否存在

```
array_key_exists(0, $arr);
```

| | PHP | TinyPHP |
|---|---|---|
| int key | ✅ | ✅ |
| string key | ✅ | ✅ |
| null key | 转空字符串 | 未定义行为 |

**差异**：int 键和 string 键分发到不同 C 函数（`_int` / `_str`）。

---

### `array_keys($arr)` — 取所有键

```
$keys = array_keys($arr);
```

| | PHP | TinyPHP |
|---|---|---|
| 返回 int 键数组 | ✅ | ✅ |
| 返回 string 键（堆拷贝） | ✅ | ✅ |
| 支持搜索值过滤 | ✅ | ❌ |

**差异**：仅基础形式。返回新数组，内存通过资源追踪管理。

---

### `array_values($arr)` — 取所有值

```
$vals = array_values($arr);
```

| | PHP | TinyPHP |
|---|---|---|
| 返回新数组 | ✅ | ✅ |
| 重排索引 | ✅ | ✅（int key 从 0 开始） |

**差异**：无。返回新数组。

---

### `array_merge($a, $b)` — 合并两个数组

```
$merged = array_merge([1,2], [3,4]);
```

| | PHP | TinyPHP |
|---|---|---|
| 两个参数 | ✅ | ✅ |
| int key 重新索引 | ✅ | ✅ |
| string key 保留 | ✅ | ✅ |
| 多参数 | ✅ | ❌（仅两个） |

**差异**：仅支持两个参数。原数组不变，返回新数组。

---

### `implode($glue, $arr)` / `join($glue, $arr)` — 连接为字符串

```
echo implode(",", [1, 2, 3]);  // "1,2,3"
```

| | PHP | TinyPHP |
|---|---|---|
| 分隔符 | ✅ | ✅ |
| int 元素自动转字符串 | ✅ | ✅ |
| float 元素 | ✅ | ✅（`%g` 格式） |

**差异**：返回堆分配 `t_string`（≤512 字节使用字符串池）。

---

### `explode($delim, $str)` — 切分为数组

```
$parts = explode(",", "a,b,c");
```

| | PHP | TinyPHP |
|---|---|---|
| 单字符分隔符 | ✅ | ✅ |
| 多字符分隔符 | ✅ | ✅ |
| 空分隔符 | 按字符切分 | 整个字符串为一个元素 |
| 限制数量参数 | ✅ | ❌ |

**差异**：片段通过字符串池分配（≤512 字节），减少 `malloc`。

---

## 控制流程

### `exit($code)` / `die($code)` — 终止程序

```
exit(0);
exit(1);
die("error");
```

| | PHP | TinyPHP |
|---|---|---|
| 整数参数 | ✅ | ✅ |
| 字符串参数 | ✅（输出后退出） | ✅（仅退出码，不输出） |
| 无参数 | ✅ (`exit()` = `exit(0)`) | ✅ |

**差异**：TinyPHP `exit("msg")` 不打印消息，只取退出码。

---

### `error($msg)` — 报错并安全退出

```
error("something went wrong");
```

| | PHP | TinyPHP |
|---|---|---|
| 存在 | ❌（无此函数） | ✅ |
| 输出格式 | — | `Fatal error: msg\n  in file.php on line N` |
| 资源清理 | — | ✅ 释放所有对象/数组/字符串 |
| 退出码 | — | 1 |

**差异**：这是 TinyPHP 专属函数，PHP 无等价物。用于替代 `throw` 实现致命错误处理。

---

## 变量检测

### `isset($x)` — 变量非 null

```
isset($x);
```

| | PHP | TinyPHP |
|---|---|---|
| 未定义变量 | false | 编译错误 |
| null | false | false |
| 0 / "" / false | true | true |
| 多参数 | ✅ | ❌ |

**差异**：TinyPHP 变量必须声明后才能 `isset`。对 int/float/bool/string 栈值始终 `true`。

---

### `empty($x)` — PHP 假值检测

```
empty($x);
```

| | PHP | TinyPHP |
|---|---|---|
| null | true | true |
| 0 | true | true |
| "" | true | true |
| "0" | true | ❌（视为非空字符串） |
| false | true | true |
| [] | true | true |

**差异**：`empty("0")` 在 TinyPHP 返回 `false`（"0" 是长度为 1 的字符串，不是假值）。

---

### `unset($x)` — 释放变量

```
unset($x);
```

| | PHP | TinyPHP |
|---|---|---|
| int | 变 0 | 变 0 |
| string | 变空 | 变 `{NULL, 0}` |
| array | 释放 | 回收到数组池 + 设 NULL |
| object | 释放 | `__destruct` + `free` |

**差异**：数组回收到复用池而非直接 `free`。

---

### 类型检测系列：`is_int` / `is_float` / `is_string` / `is_bool` / `is_array` / `is_null` / `is_object` / `is_callable`

```
is_int($x);
is_array($arr);
is_object($obj);
```

| | PHP | TinyPHP |
|---|---|---|
| 静态类型 | 运行时判断 | **编译期常量折叠**（如 `is_int(42)` → `true`） |
| `t_var` (mixed/union) | — | 运行时 `tphp_fn_is_*` 检查标签 |
| 对象类型 | ✅ | ✅（类名不在基本类型列表 → true） |

**差异**：静态类型变量在编译期直接返回 `true`/`false`，零运行时开销。

---

## 解构

### `list($a, $b)` — 数组解构赋值

```
list($a, $b) = [1, 2];
list(, $b) = [1, 2];          // 跳过元素
list($a, list($b)) = ...;     // 嵌套解构
[$a, $b] = [1, 2];            // 短语法 (PHP 7.1+)
```

| | PHP | TinyPHP |
|---|---|---|
| 基础解构 | ✅ | ✅ |
| 跳过 `list(,$b)` | ✅ | ✅ |
| 多余元素忽略 | ✅ | ✅ |
| 嵌套 `list()` | ✅ | ✅ 支持 3 层 |
| 短语法 `[$a,$b]` | ✅ | ✅ |
| 键名解构 `["key" => $v]` | ✅ (7.1+) | ✅ |

**差异**：不支持键名解构。内存安全：生成临时数组指针，读取后立即释放。

---

## JSON

### `json_encode($val)` — JSON 序列化

```
$s = json_encode([1, 2, 3]);   // "[1,2,3]"
$s = json_encode(true);        // "true"
$s = json_encode("hello");     // "\"hello\""
```

| | PHP | TinyPHP |
|---|---|---|
| null / bool / int / float / string | ✅ | ✅ |
| 数组（纯 int key） | ✅ | ✅（`[1,2,3]`） |
| 数组（含 string key） | ✅ | ✅（自动检测 → `{"k":"v"}`） |
| 嵌套数组 | ✅ | ✅（递归，受 C 栈限制，实测数千层） |
| 字符串转义 `" \n \t \\` | ✅ | ✅ |
| 对象 | ✅ | ❌（输出 `{}`） |
| JSON 美化/选项 | ✅ | ❌ |

**差异**：float 使用 `%.14g` 格式避免 `3.1400000000000001` 精度尾巴。对象序列化不支持（输出 `{}`）。

---

### `json_decode($str)` — JSON 解析

```
$v = json_decode("[1,2,3]");       // mixed (t_var)
$v = json_decode('{"x":1}');       // t_var → is_array($v) = true
$v = json_decode("not json");      // Fatal error（无效格式 abort）
```

| | PHP | TinyPHP |
|---|---|---|
| null / bool / int / float / string | ✅ | ✅ |
| 数组 `[...]` | ✅ | ✅ |
| 对象 `{...}` | ✅ | ✅（存为 t_array with string keys） |
| 嵌套 | ✅ | ✅（递归，无硬编码深度限制） |
| 字符串转义 `\n \t \\ \" \uXXXX` | ✅ | ✅（`\u` → `?`） |
| 格式错误安全返回 | ✅（返回 null） | ✅（完全无效 abort，部分解析返回 NULL + 释放内存） |
| 截断输入 | ✅ | ✅（`[1,2,` → NULL） |

**差异**：返回类型为 `t_var`（mixed），需 `is_array`/`is_int` 等运行时检测。`\uXXXX` 简单映射为 `?`。

---

## 常量

### `const` — 三种作用域常量

**全局常量**（任意 `.php` 文件顶层）：
```
const APP_NAME = "MyApp";
const MAX     = 100;
```

**命名空间常量**（`namespace Foo;` 内）：
```
namespace Lib;
const VERSION = "1.0";
```

**类常量**（`class` 体内）：
```
class Demo {
    const string AAA = "hello";           // 隐式 public
    public const int TIMEOUT = 30;
    private const bool DEBUG = false;
    private const array TAGS = ["web"];
}
```

| | PHP | TinyPHP |
|---|---|---|
| 全局 `const X = val` | ✅ | ✅（`#define`） |
| 命名空间 `const X = val` | ✅ | ✅ |
| 类 `const TYPE X = val` | ✅ | ✅ |
| `public/private const` | ✅ | ✅ |
| `string/int/float/bool/array` 类型 | ✅ | ✅ |
| `self::CONST` 类内访问 | ✅ | ✅ |
| `ClassName::CONST` 外部访问 | ✅ | ✅（public 允许，private 报错） |
| 跨文件常量引用 | ✅ | ✅（`constTypes` 记录类型） |
| `const` 无类型标注 | ✅ | ✅（全局） |

**差异**：全局/命名空间常量无类型标注（`const X = 1`），类常量需类型标注（`const int X = 1`）。

---

## 时间 / 日期

### `time()` — 当前 Unix 时间戳

```
$ts = time();
```

| | PHP | TinyPHP |
|---|---|---|
| 返回类型 | int | `t_int` (int64_t) |
| 精度 | 秒 | 秒 |

**差异**：无。直接 `time(NULL)`，零堆分配。

---

### `date($format, $timestamp?)` — 格式化时间

```
echo date("Y-m-d H:i:s", 1717200000);
echo date("Y");                    // 当前时间
```

| | PHP | TinyPHP |
|---|---|---|
| 格式字符 | 30+ | Y y m n d j H G i s（8 个） |
| 默认时间戳 | 当前时间 | 当前时间 |
| 时区 | php.ini | 系统本地时区 |
| 内存分配 | 堆 | 静态缓冲区（零分配） |

**差异**：仅支持 8 个格式字符。手写解析，跨平台一致。

---

### `sleep($seconds)` — 休眠

```
sleep(1);      // 1 秒
sleep(0);      // 无操作
```

| | PHP | TinyPHP |
|---|---|---|
| 负数 | 报错 | 静默忽略 |
| 精度 | 秒（int） | 秒（int） |

**差异**：负数不报错，静默返回。

---

### `usleep($microseconds)` — 微秒休眠

```
usleep(500000);   // 0.5 秒
usleep(1000);     // 1 毫秒
```

| | PHP | TinyPHP |
|---|---|---|
| 精度 | 微秒 | 微秒 |
| Windows 实现 | `usleep` 模拟 | `Sleep(ms)`（毫秒精度） |

**差异**：Windows 上精度退化为毫秒。

---

### `hrtime()` — 高精度时间（纳秒）

```
$ns = hrtime();
$start = hrtime();
// ... do work ...
$elapsed = hrtime() - $start;
```

| | PHP | TinyPHP |
|---|---|---|
| 参数 `true` 返回数组 | ✅ | ❌（无参数） |
| 返回值 | int/array | `t_int`（纳秒） |
| Windows 实现 | `QueryPerformanceCounter` | 同 |
| Linux/macOS 实现 | `clock_gettime(CLOCK_MONOTONIC)` | 同 |

**差异**：仅支持无参形式，返回纳秒整数。零堆分配。

---

## 实现特性总结

| 特性 | 说明 |
|---|---|
| **数组对象池** | 128 槽 LIFO 复用池 + 1.5× 增长因子 |
| **小字符串池** | 64KB bump allocator，≤512B 零 `malloc` |
| **分支预测** | `likely`/`unlikely` 标注所有热路径 |
| **编译期类型折叠** | `is_int(42)` 等静态类型编译期求值 |
| **嵌套类型追踪** | 2 层数组自动追踪元素类型 |
| **JSON 编解码** | 基本类型+数组+对象+转义，无效 JSON → `error()` |
| **常量三作用域** | 全局/命名空间/类常量，`self::` / `Class::` 访问 |
| **键名解构** | `["key" => $v]` 支持（PHP 7.1+） |
| **sort/rsort** | libc `qsort` 原地排序 |
| **内置函数 50+** | 数组 18、字符串 8、类型 8、转换 4、通用 6、时间 5、JSON 2 |
| **PHPC 互操作** | 数组/对象/回调 ↔ C 双向转换，`#include`/`#flag` 编译器集成 |
| **闭包堆捕获** | `use` 变量 `calloc` 到堆，`tphp_rt_register(type=3)`，内存安全 |
| **编译标志** | `#flag` 按平台+编译器过滤，`#include` 支持 `""`/`<>`，自动去重 |
| **error 安全退出** | 遍历资源链表释放所有对象/数组/字符串/闭包环境 |
| **跨平台** | TCC/GCC/Clang 编译通过，`#ifdef _WIN32` |
| **PHAR 自包含** | `tphp.phar` 内嵌 TCC + 头文件，单文件分发 |

---

## 字符串

### `strlen($s)` — 字符串长度

```
$len = strlen("hello");   // 5
```

| | PHP | TinyPHP |
|---|---|---|
| 返回类型 | int | `t_int` |
| null 输入 | 报错 | 返回 0 |
| 空字符串 | 0 | 0 |

**差异**：直接返回 `s.length`，零开销。

---

### `trim($s)` / `ltrim($s)` / `rtrim($s)` — 去除空白

```
trim("  hi  ");    // "hi"
ltrim("  hi  ");   // "hi  "
rtrim("  hi  ");   // "  hi"
```

| | PHP | TinyPHP |
|---|---|---|
| 空白字符 | ` \t\n\r\0\x0B` | `<= 0x20`（ASCII 控制字符） |
| 自定义字符集 | ✅ | ❌ |
| 内存 | 堆 | `str_pool_alloc` |

**差异**：仅去除 ASCII 空白控制字符（`<= ' '`），不支持自定义字符集。

---

### `substr($s, $offset, $length?)` — 子串

```
substr("hello", 1, 3);   // "ell"
substr("hello", -2, 0);  // "lo"
substr("hello", 0, -1);  // "hell"
```

| | PHP | TinyPHP |
|---|---|---|
| 负数 offset | ✅ | ✅ |
| 负数 length | ✅（从末尾去掉 N 字符） | ✅ |
| length=0 | ✅（到末尾） | ✅ |
| 越界 | 空字符串 | 空字符串 |

**差异**：负数 length 行为与 PHP 一致。内存通过 `str_pool_alloc` 分配。

---

### `strpos($haystack, $needle)` — 查找子串位置

```
strpos("hello", "ll");   // 2
strpos("hello", "xx");   // -1 (PHP: false)
```

| | PHP | TinyPHP |
|---|---|---|
| 未找到 | `false` | `-1` |
| 空 needle | 报错 | 返回 0 |
| 偏移参数 | ✅ | ❌ |

---

### `str_contains($haystack, $needle)` — 是否包含子串

```
str_contains("hello", "ll");   // true
str_contains("hello", "xx");   // false
```

调用 `strpos ≥ 0` 实现，返回 `t_bool`。

---

### `str_replace($search, $replace, $subject)` — 子串替换

```
str_replace("a", "X", "abcabc");  // "XbcXbc"
```

| | PHP | TinyPHP |
|---|---|---|
| 全部替换 | ✅ | ✅ |
| 数组参数 | ✅ | ❌ |
| 计数参数 | ✅ | ❌ |

**实现**：两遍扫描（计数 + 构建），`str_pool_alloc` 分配新 buffer。

---

## 数组（续）

### `array_shift($arr)` — 头部弹出

```
$val = array_shift($arr);
```

| | PHP | TinyPHP |
|---|---|---|
| 空数组 | null | NULL（`VAR_NULL()`） |
| 返回类型 | mixed | `t_var` |
| 性能 | O(1) | O(n) memmove |

**内存安全**：释放弹出 entry 的 string key，memmove 左移剩余元素。

---

### `array_unshift($arr, $val)` — 头部追加

```
$len = array_unshift($arr, 99);
```

| | PHP | TinyPHP |
|---|---|---|
| 返回值 | 新长度 | 新长度 |
| 多值追加 | ✅ | ❌（仅单值） |

memmove 右移 + 重编号 int key。

---

### `array_sum($arr)` / `array_product($arr)` — 求和/求积

```
$sum = array_sum([1, 2, 3]);       // int(6)
$prod = array_product([1, 2, 3]);   // int(6)
```

| | PHP | TinyPHP |
|---|---|---|
| int 元素 | ✅ | ✅ |
| float 元素 | 自动提升为 float | ✅ |
| 空数组 | sum=0, product=1 | 同 |
| 返回类型 | int/float | `t_var` |

**差异**：只要有一个 float 元素，结果自动提升为 float。

---

### `array_unique($arr)` — 去重

```
$u = array_unique([1, 2, 2, 3]);  // [1, 2, 3]
```

| | PHP | TinyPHP |
|---|---|---|
| 保留首次出现 | ✅ | ✅ |
| 比较类型 | = 松散比较 | 严格类型+值比较 |
| SORT 选项 | ✅ | ❌ |

O(n²) 双重循环，新建数组，不修改原数组。

---

### `array_reverse($arr, $preserve_keys?)` — 反转

```
$r = array_reverse([1, 2, 3]);  // [3, 2, 1]
```

倒序遍历新建数组。`preserve_keys=true` 时 string key 深拷贝。

---

### `array_slice($arr, $offset, $length?, $preserve_keys?)` — 切片

```
array_slice([10,20,30,40], 1, 2);  // [20, 30]
array_slice([10,20,30,40], -2, 0); // [30, 40]
```

| | PHP | TinyPHP |
|---|---|---|
| 负数 offset | ✅ | ✅ |
| length=0 | 到末尾 | ✅ |
| preserve_keys | ✅ | ✅ |

---

## 通用

### `max($arr)` / `min($arr)` — 最值

```
$mx = max([5, 99, 3]);   // 99
$mn = min([5, 99, 3]);   // 3
```

| | PHP | TinyPHP |
|---|---|---|
| 空数组 | Warning | `error()` 退出 |
| 非数值 | 跳过 | 跳过 |
| 多参数 | ✅ | ❌（仅数组） |
| 返回类型 | mixed | `t_var` |

---

### `range($start, $end, $step?)` — 范围数组

```
range(1, 5);       // [1, 2, 3, 4, 5]
range(10, 1, -3);  // [10, 7, 4, 1]
```

| | PHP | TinyPHP |
|---|---|---|
| step=0 | ValueError | `error()` 退出 |
| 返回类型 | array | `t_array*` |

预知长度一次分配全部 entry，零 realloc。

---

### `array_fill($start_index, $count, $value)` — 填充数组

```
array_fill(0, 3, 99);  // [99, 99, 99]
```

`count < 0` → `error()` 退出。通过 `set_int` 设置指定 key。

---

### `sort($arr)` / `rsort($arr)` — 原地排序

```
sort([30, 10, 20]);   // [10, 20, 30]
rsort([30, 10, 20]);  // [30, 20, 10]
```

libc `qsort` 原地排序，重编号 int key。比较 int/float 值，忽略非数值类型。

---

### `sprintf($fmt, ...$args)` — 格式化字符串

```
sprintf("Hi %s", "Alice");          // "Hi Alice"
sprintf("Age: %d", 30);             // "Age: 30"
sprintf("%s is %d", "Bob", 25);    // "Bob is 25"
```

| | PHP | TinyPHP |
|---|---|---|
| `%s` `%d` `%f` | ✅ | ✅ |
| 可变参数 | ✅ | ✅ |
| `%02d` 等格式标记 | ✅ | ✅ |
| 完整 `printf` 语法 | ✅ | ✅（委托 snprintf，全格式支持） |

**内存安全**：`snprintf(NULL,0,...)` 动态测量 → `str_pool_alloc` 精确分配，无上限、无截断。

---

## 魔术常量

### `__LINE__` / `__FILE__` / `__DIR__` — 编译期常量

```
echo __LINE__;    // 当前行号
echo __FILE__;    // 完整文件路径
echo __DIR__;     // 文件所在目录
```

| | PHP | TinyPHP |
|---|---|---|
| 编译期替换 | runtime | **编译期常量**（AOT 天然支持） |
| 跨文件引用 | ✅ | ✅ |
| `__DIR__` 等价 `dirname(__FILE__)` | ✅ | ✅ |

### `DIRECTORY_SEPARATOR` — 目录分隔符

```
echo DIRECTORY_SEPARATOR;  // "/" (Linux/macOS) 或 "\" (Windows)
```

编译期替换为 `STR_LIT("/")` 或 `STR_LIT("\\")`。

---

## C 互操作 (PHPC)

> 所有 PHPC 函数为**全局函数**，不受命名空间 `namespace` 影响。通过 `common.h` 自动包含 `phpc.h`（~180 行运行时）。测试：`test/phpc/`。

### `#include` — 引入 C 头文件

```php
#include "include/demo.h"    // 项目头文件 → #include "include/demo.h"
#include <math.h>             // 系统头文件 → #include <math.h>
```

| 格式 | C 输出 | 用途 |
|------|--------|------|
| `#include "path"` | `#include "path"` | 项目相对路径头文件 |
| `#include <name>` | `#include <name>` | 系统头文件 |

多次相同文件自动去重。头文件所在目录自动添加为 `-I` 路径，同目录 `.c` 文件自动联编。

### `#flag` — 编译器标志

```php
#flag -DNDEBUG                // 全平台 + 全编译器
#flag Linux -lm               // 仅 Linux
#flag GCC -O2                 // 仅 GCC
#flag Clang -O2 -Wall         // 仅 Clang
#flag Linux GCC -march=native // Linux + GCC 组合
```

| 过滤器 | 值 |
|--------|-----|
| 平台 | `Windows`, `Linux`, `MacOS` |
| 编译器 | `GCC`, `Clang`, `TCC` |
| 默认 | 不写 = 全平台 + 全编译器 |

重复标志串自动去重。最多两个前缀（编译器 + 平台），顺序不限。

### `C->function()` — 直接 C 调用

```php
C->calc_distance(c_float($x1), c_float($y1), c_float($x2), c_float($y2))
// 生成: calc_distance(c_float(x1), c_float(y1), c_float(x2), c_float(y2))
```

无 `tphp_` 前缀，无命名空间 mangle。C 函数必须存在于已 `#include` 的头文件中。

### 类型桥接 (`phpc.h`)

```php
$dist = php_float(C->calc_distance(   // C double → PHP float
    c_float($x1), c_float($y1),        // PHP float → C double
    c_float($x2), c_float($y2)
));
```

| PHP → C | C 类型 | C → PHP | PHP 类型 |
|----------|--------|----------|----------|
| `c_int($x)` | `int32_t` | `php_int($v)` | `t_int` |
| `c_float($x)` | `double` | `php_float($v)` | `t_float` |
| `c_str($s)` | `const char*` | `php_str($s)` | `t_string`（深拷贝） |

`c_str` 返回 `t_string.data`（不拷贝），`php_str` 对 C 字符串做 `tphp_rt_str_dup`。

### 数组互操作

**严格 C 风格类型检查**：`phpc_arr_int` 要求所有元素为 `TYPE_INT`，不匹配则 `error()` 退出。`phpc_arr_dbl` 接受 `int`/`float`。

```php
function sum_array(array $arr): int {
    $data = phpc_arr_int($arr);                      // → int32_t* (malloc)
    $result = C->sum_ints($data, c_int(count($arr))); // C 操作
    phpc_free($data);                                 // 必须释放！
    return php_int($result);
}

function double_all(array $arr): array {
    $len = count($arr);
    $data = phpc_arr_int($arr);           // 提取
    C->double_each($data, c_int($len));   // C 原地修改
    $out = phpc_new_arr_int($data, $len); // 深拷贝回 PHP
    phpc_free($data);                     // 释放
    return $out;
}
```

| PHP → C | 类型要求 | 返回 |
|----------|----------|------|
| `phpc_arr_int($arr)` | 全部 TYPE_INT | `int32_t*` (malloc) |
| `phpc_arr_dbl($arr)` | TYPE_INT 或 TYPE_FLOAT | `double*` (malloc) |
| `phpc_arr_str($arr)` | 全部 TYPE_STRING | `char**` (malloc，逐元素分配) |

| C → PHP | 说明 |
|----------|------|
| `phpc_new_arr_int(src, len)` | `int32_t[]` → `t_array*`（深拷贝） |
| `phpc_new_arr_dbl(src, len)` | `double[]` → `t_array*` |
| `phpc_new_arr_str(src, len)` | `char*[]` → `t_array*` |
| `phpc_new_arr()` | 空 `t_array*` |

### 对象互操作

TinyPHP 对象 = `t_object` 头部（vtable + refcount）+ 字段。`phpc_obj` 返回底层结构体指针：

```php
class MyPoint { public float $x; public float $y; }

function read_x(MyPoint $p): float {
    $ptr = phpc_obj($p);                 // → void* (即 tphp_class_MyPoint*)
    return php_float(C->read_field($ptr, c_int(16))); // offsetof(x) = sizeof(t_object)
}
```

| 函数 | 方向 | 签名 | 说明 |
|------|------|------|------|
| `phpc_obj($obj)` | PHP→C | `t_object* → void*` | 提取底层 C 结构体指针 |
| `phpc_new_obj(ptr, vtable)` | C→PHP | `void*, ClassVTable* → t_object*` | 包裹 C 指针，vtable 管理析构 |

`phpc_new_obj` 也会调用 `tphp_rt_register(ptr, 0)`，`error()` 时自动析构。

### 回调互操作

**有 env 回调** — C 签名含 `void* env`：

```php
$square = function(int $x): int { return $x * $x; };
$result = C->apply_closure(
    phpc_fn_i32($square),  // → int32_t(*)(int32_t, void*) 类型安全
    phpc_env($square),     // → void* (env)
    c_int(5)
);
```

| 函数 | 返回类型 | 说明 |
|------|---------|------|
| `phpc_fn_i32($cb)` | `int32_t(*)(int32_t, void*)` | int32 回调指针 |
| `phpc_fn_i64($cb)` | `int64_t(*)(int64_t, void*)` | int64 回调指针 |
| `phpc_fn_f64($cb)` | `double(*)(double, void*)` | double 回调指针 |
| `phpc_fn($cb)` | `void*` | 通用（需手动 cast） |
| `phpc_env($cb)` | `void*` | 捕获环境指针 |

**无 env 回调** — `#callback` 声明 + `phpc_thunk()`：

```php
// 1. 声明 C 回调签名（任意参数/类型）
#callback double fold_cb(int32_t idx, double val)

// 2. thunk 嵌入 env，签名精确匹配
C->fold_dbl($data, $len, phpc_thunk('fold_cb', $fn));
// 生成: static double _thunk_N(int32_t idx, double val) { ... env嵌入 ... }
```

| 函数 | 说明 |
|------|------|
| `phpc_thunk('name', $fn)` | 按 #callback 声明签名生成 thunk |
| `phpc_new_fn(func)` → `t_callback` | C 函数指针包装 |
| `phpc_new_fn_env(func, env)` | 带环境版本 |

### 内存释放

| 函数 | 说明 |
|------|------|
| `phpc_free(ptr)` | `free(ptr)`，NULL 安全 |
| `phpc_free_str_arr(strs, len)` | 逐个 `free(strs[i])` → `free(strs)` |

**关键规则**：`phpc_arr_*` 返回 `malloc` 指针，必须通过 `phpc_free`/`phpc_free_str_arr` 释放。`phpc_new_arr_*` 返回 `t_array*` 由 TinyPHP 引用计数自动管理。

---

## 后续建议实现

### 低难度（⭐）
| 函数/语法 | 说明 |
|---|---|
| `define("CONST", val)` | 等同于 `const CONST = val` |
| `intl/ctype` 函数 | `isalpha`/`strtolower` 等映射 libc |

### 中等难度（⭐⭐）
| 函数/语法 | 说明 |
|---|---|
| `try/catch/finally` | `setjmp/longjmp` + 资源栈 |
| `throw new Exception($msg)` | 需内置 Exception 类 |
| `file_get_contents`/`file_put_contents` | 文件 I/O |
| 类继承 `extends` | VTable 扩展 |
| 接口 `interface`/`implements` | VTable 契约 |
| Trait | 编译期方法扁平化 |
| Generators `yield` | 状态机 |

### 较高难度（⭐⭐⭐）
| 函数/语法 | 说明 |
|---|---|
| `preg_match`/`preg_replace` | 需嵌入 PCRE2 ~200KB |
| `array_map`/`array_filter` | 回调类型规范化 |
| 属性 Hook `{ get => ... }` | PHP 8.4 |

### AOT 不可行

| 特性 | 原因 |
|---|---|
| `eval($code)` | 需要运行时 PHP 解析器 |
| `include $dynamicPath` / `require` | 编译期路径不可知 |
| `$$var` 可变变量 | 编译期无法确定符号 |
| `$obj->$prop` / `new $class()` | 运行时动态解析 |
| `extract()` / `compact()` | 动态符号表操作 |
| `Reflection*` / `get_class_methods()` | 需运行时元数据 |
| `serialize` / `unserialize` | 需类型反射 |
| `__call` / `__get` / `__set` 魔术方法 | 运行时分发 |
| `Closure::bind()` / `Closure::fromCallable()` | 运行时作用域绑定 |
| `preg_match` / PCRE 正则 | 需嵌入 ~200KB 库 |
| `PDO` / `mysqli` | 需外部链接 |
