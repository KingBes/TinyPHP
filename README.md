# TinyPHP

> PHP → C 转译编译器，将 PHP 子集转为安全 C 代码，由编译器编译为原生产物。

## 快速开始

```bash
# 编译单文件
php tphp.php test/var/var.php

# 编译多文件
php tphp.php main.php demo.php lib/helper.php

# 扫描当前目录所有 .php 编译
php tphp.php .

# 指定输出
php tphp.php main.php -o app

# 指定编译器（默认内置 TCC）
php tphp.php main.php -cc gcc
php tphp.php main.php -cc clang
```

Linux/macOS 还可用快捷入口 `./tphp`：

```bash
./tphp test/var/var.php
```

## 编译流水线

```
PHP 源码 → Lexer → Token[] → Parser → AST → CodeGenerator → .c → 编译器 → 产物
```

- `src/Lexer.php` — 词法分析，逐字符扫描生成 Token 流
- `src/Parser.php` — 递归下降解析，构建 AST
- `src/CodeGenerator.php` — 访问者模式生成 C 代码
- TCC 从 `https://repo.or.cz/tinycc.git` (mob 分支) 实时构建

## 性能

纯数组读写场景下总体 **3~4x** 快于 PHP 8.x（详见 [bench_array.php](test/var/bench_array.php)）：

| 场景 | TinyPHP | PHP 8.5 | 比率 |
|---|---|---|---|
| 1000 元素 ×100K 循环读取 | 389 ms | 1638 ms | **4.2x 快** |
| count + 遍历 ×100K | 41 ms | 160 ms | **3.9x 快** |
| 5 元素创建 ×100K | 60 ms | 1.9 ms | 31x 慢 * |

\* 创建开销来自 C `malloc`，PHP 用 slab 分配器几乎零开销，后续优化方向。

## 支持的语言特性

### 类型系统

| PHP | C |
|-----|---|
| `int` | `int64_t` |
| `float` | `double` |
| `string` | `struct { char *data; int length; }` |
| `bool` | `bool` |
| `null` | `void *` |
| `array` | `t_array *`（有序映射，int 键，嵌套） |
| `callable` | `t_callback { void *func; void *env; }` |
| `mixed` / `int\|string` | `t_var`（类型标签 union） |

### 运算符

| 类别 | 运算符 |
|------|--------|
| 算术 | `+` `-` `*` `/` `%` `**` |
| 比较 | `==` `!=` `<` `>` `<=` `>=` `<=>` |
| 逻辑 | `&&` `\|\|` `!` |
| 位运算 | `&` `\|` `^` `~` `<<` `>>` |
| 字符串 | `.` |
| 复合赋值 | `=` `+=` `-=` `*=` `/=` `.=` |
| 自增/自减 | `++` `--`（前缀+后缀） |
| 三元/合并 | `?:` `??` |
| 类型转换 | `(int)` `(float)` `(string)` `(bool)` `(array)` |

### 语法支持

```php
class Main {
    public function main(): void {
        // 变量
        $a = 10; $b = "hello"; $c = true; $d = 1.01; $e = null;

        // 运算符: + - * / % ** . && || ! == != < > <= >= <=>
        // 复合赋值: = += -= *= /= .=
        // 自增自减: ++$i $i++ --$i $i--
        // 三元 & 合并: ?:  ??
        // 位运算: & | ^ ~ << >>
        $sum = $a + 5; $a += 10; $a++;
        $r = 2 ** 10;                  // 幂运算
        $cmp = $a <=> $b;              // 太空船
        $x = $a > 0 ? "yes" : "no";
        $y = $maybeNull ?? "default";

        // 字符串拼接 & 插值
        $s = $a . " " . $b;
        $s2 = "hello $d\n";
        $s3 = "hello {$d}\n";
        $s4 = "{$this->name} ok";      // 花括号内 →prop 支持

        // 数组
        $arr = [1, 2, 3];
        $map = ["key" => "val", "nested" => [4, 5]];
        $x = $arr[0]; $arr[0] = 42;
        count($arr);

        // 控制流
        if ($a > 0) { } elseif ($a == 0) { } else { }
        while ($i < 10) { $i++; }
        do { $i++; } while ($i < 10);
        for ($i = 0; $i < 10; $i++) { }
        foreach ($arr as $v) { }
        foreach ($map as $k => $v) { }
        switch ($v) { case 1: break; default: break; }
        match ($v) { 1 => "one", default => "other" };
        break; continue; goto label;

        // 输出 & 调试
        echo "hello\n";
        var_dump($a);
        exit(0);

        // 对象 & 链式调用
        $d = new Demo(); $d->hello();
        $calc->add(5)->multiply(3);

        // 闭包 & 变量捕获
        $fn = function (int $x): int { return $x * 2; };
        $capture = function (int $x) use ($m): int { return $x * $m; };

        // 类型转换
        $s = (string)123; $i = (int)"456"; $f = (float)"1.2";

        // 系统函数
        $ts = time();
        echo date("Y-m-d H:i:s", $ts);
        sleep(1); usleep(500000);
        $ns = hrtime();

        // 错误处理
        error("something went wrong");
    }
}
```

### 枚举

```php
enum Color: string {
    case RED = "red";
    case GREEN = "green";
}
enum Num: int {
    case ONE = 1; case TWO = 2;
}
```

### 多文件 & 命名空间

```php
// main.php — 入口（必须有全局 class Main）
use Demo\Demo;
use function Demo\myFunc;

// demo.php — 命名空间
namespace Demo;
class Demo { ... }
function myFunc(): void { ... }
```

### 内置函数

| 函数 | 说明 |
|------|------|
| `echo` | 输出字符串 |
| `var_dump` | 递归调试输出 |
| `count` | 数组元素计数 |
| `exit($code)` / `die($code)` | 终止程序 |
| `isset($x)` | 变量非 null |
| `empty($x)` | PHP 假值检测 |
| `list($a,$b)` | 数组解构（支持跳过/嵌套/短语法 `[]`） |
| `unset($x)` | 释放变量 |
| `time()` | Unix 时间戳 |
| `date($fmt, $ts?)` | 格式化时间 |
| `sleep($s)` | 休眠秒数 |
| `usleep($us)` | 休眠微秒 |
| `hrtime()` | 高精度纳秒 |
| `error($msg)` | 报错 + 清资源 + 退出 |

> 详见 [FUNCTIONS.md](FUNCTIONS.md) — 每个函数与 PHP 的差异对照及后续实现建议。
> 性能优化路线：[ROADMAP.md](ROADMAP.md)

### 控制流完整列表

`if` / `elseif` / `else` · `while` · `do-while` · `for` · `foreach` · `switch` / `case` / `default` · `break` · `continue` · `goto` · `match`

## 构建 TCC

```bash
# Windows（需 MSYS2 MinGW64）
.\build.cmd

# Linux / macOS
bash build.sh
```

## C 运行时 (`include/`)

| 文件 | 内容 |
|------|------|
| `common.h` | 总入口 |
| `types.h` | 类型系统：t_int, t_float, t_string, t_bool, t_var, t_array, t_object |
| `val.h` | 便捷宏：VAR_INT, STR_LIT, VAR_AS_* |
| `array.h` | PHP 数组：引用计数 + 嵌套释放 + 对象池优化 |
| `runtime.h` | 内部辅助：字符串拼/拆/比较、资源追踪、error 清理 |
| `builtin.h` | 公开内置：echo, var_dump, exit, isset, empty |
| `os/times.h` | 系统函数：time, date, sleep, usleep, hrtime |

## CLI 选项

| 选项 | 说明 |
|------|------|
| `-o <output>` | 输出文件路径 |
| `-cc <compiler>` | 指定 C 编译器 |
| `-h, --help` | 显示帮助 |

## 许可证

MIT
