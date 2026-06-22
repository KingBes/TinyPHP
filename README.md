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

读/遍历场景总体 **3~5x** 快于 PHP 8.x：

| 场景 | TinyPHP | PHP 8.x | 比率 |
|---|---|---|---|
| foreach 1K ×100K 循环 | 581 ms | 1,885 ms | **3.2× 快** |
| count + for ×100K | 42 ms | 228 ms | **5.4× 快** |
| 嵌套数组读 ×100K | 1.2 ms | 3.9 ms | **3.2× 快** |
| int key 读取 ×100K | 1.1 ms | 3.0 ms | **2.7× 快** |
| array_pop ×100K | 2.3 ms | 4.3 ms | **1.8× 快** |
| 数组创建（push 1000×100） | 4.1 ms | 1.8 ms | 2.3× 慢* |
| explode+implode ×10K | 24.7 ms | 2.5 ms | 9.9× 慢* |

\* 创建开销来自 C `malloc`，PHP 用 slab 分配器。GCC/Clang 编译可进一步改善。详见 [bench_tphp.php](test/var/bench_tphp.php)。

## 支持的语言特性

### 近期更新

| 日期 | 更新 |
|------|------|
| 2026-06 | **闭包捕获变量**：堆分配捕获环境 + 资源追踪，`t_var`/对象/`null` 等全部类型正确推导，`unset` 安全释放 |
| 2026-06 | **for 循环作用域提升**：`for($i=0;…)` 声明的变量自动提升到函数作用域，循环后仍可访问 |
| 2026-06 | **foreach 字符串 key**：`foreach($map as $k=>$v)` 自动检测字符串键，`$k` 类型为 `t_string` |
| 2026-06 | **match 无 default 安全**：无 `default` 分支时自动零值初始化，防止未定义行为 |
| 2026-06 | **数组/参数尾部逗号**：`[1,2,]` 和 `foo(1,2,)` 尾部逗号解析支持 |
| 2026-06 | **代码生成质量**：`wrapTvarAssign`/`wrapArrayElement`/`inferType` 增强类型推导精度 |

### 类型系统

| PHP | C |
|-----|---|
| `int` | `int64_t` |
| `float` | `double` |
| `string` | `struct { char *data; int length; }` |
| `bool` | `bool` |
| `null` | `void *` |
| `array` | `t_array *`（有序映射，int/string 键，嵌套） |
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

        // 数组 & 数组函数
        $arr = [1, 2, 3];
        $map = ["key" => "val", "nested" => [4, 5]];
        $x = $arr[0]; $arr[0] = 42;
        count($arr);
        array_push($arr, 99);
        $val = array_pop($arr);
        in_array(42, $arr);
        array_key_exists(0, $arr);
        array_keys($arr); array_values($arr);
        array_merge([1,2], [3,4]);
        implode(",", $arr);
        explode(",", "a,b,c");

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

        // 类型检测（编译期常量折叠）
        is_int($a); is_string($b); is_array($arr); is_object($obj);

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

        // JSON
        $s = json_encode([1, 2, 3]);        // "[1,2,3]"
        $v = json_decode('{"x":1}');        // mixed
        $s = json_encode($v);                // "{"x":1}"

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
| `array_push` | 尾部追加 |
| `array_pop` | 尾部弹出 |
| `array_keys` | 取所有键 |
| `array_values` | 取所有值 |
| `array_merge` | 合并两个数组 |
| `in_array` | 值是否存在 |
| `array_key_exists` | 键是否存在 |
| `implode` / `join` | 连接为字符串 |
| `explode` | 切分为数组 |
| `is_int` / `is_float` / `is_string` / `is_bool` | 类型检测（编译期折叠） |
| `is_array` / `is_null` / `is_object` / `is_callable` | 类型检测（编译期折叠） |
| `exit($code)` / `die($code)` | 终止程序 |
| `isset($x)` | 变量非 null |
| `empty($x)` | PHP 假值检测 |
| `list($a,$b)` | 数组解构（支持跳过/嵌套/短语法 `[]`） |
| `unset($x)` | 释放变量（数组回收到复用池） |
| `time()` | Unix 时间戳 |
| `date($fmt, $ts?)` | 格式化时间 |
| `sleep($s)` | 休眠秒数 |
| `usleep($us)` | 休眠微秒 |
| `hrtime()` | 高精度纳秒 |
| `json_encode($val)` | JSON 序列化 |
| `json_decode($str)` | JSON 解析（返回 mixed） |
| `const TYPE NAME = val` | 常量定义（全局/命名空间/类三种作用域） |
| `self::CONST` / `Class::CONST` | 类常量访问（private 外部禁止） |
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
| `types.h` | 类型系统 + `likely`/`unlikely` 分支预测宏 |
| `val.h` | 便捷宏：VAR_INT, STR_LIT, VAR_AS_* |
| `array.h` | PHP 数组：引用计数 + 128 槽复用池 + 1.5× 增长因子 |
| `runtime.h` | 内部辅助：字符串拼/拆/比较、64KB 小字符串池、资源追踪、error 清理 |
| `builtin.h` | 公开内置：echo, var_dump, 类型检测, 数组函数, implode/explode |
| `os/times.h` | 系统函数：time, date, sleep, usleep, hrtime |

## CLI 选项

| 选项 | 说明 |
|------|------|
| `-o <output>` | 输出文件路径 |
| `-cc <compiler>` | 指定 C 编译器 |
| `-h, --help` | 显示帮助 |

## 许可证

MIT

---

## 语法待完善清单

> 以下为可实现的语法增强项，按难度排序。不包括 `eval/include/throw/try-catch/yield` 等 AOT 不支持特性。

### 低难度（⭐）

| 特性 | 说明 | 涉及文件 |
|------|------|----------|
| `===` / `!==` 严格比较 | Lexer 增加 `===` `!==` 三字符 Token，Parser/CodeGen 映射为 `==` `!=` | Lexer, TokenType |
| `?type` 可空类型标注 | `?int` → Lexer 增加 `?` Token，Parser `parseType` 识别 `?type` 语法 | Lexer, Parser |
| `&&=` `\|\|=` `??=` 复合赋值 | Lexer 增加三字符 Token，Parser `parseExprStmt` 匹配，CodeGen 生成对应 C | Lexer, Parser, CodeGen |
| `&=` `\|=` `^=` `<<=` `>>=` `%=` `**=` | 位运算复合赋值，同上流程 | Lexer, Parser, CodeGen |
| `static` 返回类型 | Parser `parseType` 识别 `static` 关键字 | Parser, CodeGen |
| `never` 返回类型 | PHP 8.1，Lexer 增加关键字 | Lexer, Parser |
| `iterable` 伪类型 | Lexer 识别 `iterable`，映射为 `t_array*` | Lexer, CodeGen |
| `readonly` 属性 | Parser 属性声明中识别 `readonly` 修饰符 | Parser, CodeGen |
| `final` 关键字 | 类/方法添加 `final` 修饰（编译期检查） | Parser, CodeGen |
| `intval/floatval/strval/boolval` | 内置类型转换函数，C 实现 ~30 行 | CodeGen, builtin.h |
| `rand/mt_rand` | 随机数函数 | CodeGen, builtin.h |
| `defined("CONST")` | 常量是否定义，编译期可知 | CodeGen |
| `@` 错误抑制符 | 表达式前缀标记，编译期忽略 | Lexer, Parser |
| 反引号执行 | `` `cmd` `` → `system(cmd)` | Lexer, CodeGen |
| `$s[0]` 字符串偏移访问 | 单字符读取 | Parser, CodeGen |

### 中等难度（⭐⭐）

| 特性 | 说明 | 涉及文件 |
|------|------|----------|
| 参数默认值 | `function foo(int $x = 10)` — Parser 解析默认值，CodeGen 在调用侧补全 | Parser, CodeGen |
| 构造器属性提升 | `__construct(public int $x)` — Parser 语法糖展开为属性声明+参数 | Parser |
| 箭头函数 `fn($x) => $x * 2` | Lexer 识别 `fn`，Parser 解析短语法，CodeGen 生成闭包 | Lexer, Parser, CodeGen |
| 可变参数 `...$args` | 声明侧和调用侧的 spread 语法 | Parser, CodeGen |
| First-class callable `$fn = strlen(...)` | PHP 8.1 语法 | Parser, CodeGen |
| 命名参数 `foo(name: "test")` | 调用侧参数名匹配 | Parser, CodeGen |
| Nullsafe `$obj?->method()` | 链式调用 null 安全 | Parser, CodeGen |
| foreach key 支持 string+int 混合 | 当前仅支持纯 string 或纯 int key 类型检测 | CodeGen |
| `strlen/substr/strpos` | 字符串操作函数，C 实现 ~60 行 | CodeGen, builtin.h |
| `trim/ltrim/rtrim` | 空白字符修剪 | CodeGen, builtin.h |
| `sprintf` | 格式化字符串 | CodeGen, builtin.h |
| `file_get_contents/file_put_contents` | 文件 I/O | CodeGen, builtin.h |
| `array_shift` | 头部弹出 | CodeGen, builtin.h |
| 枚举方法 | `enum` 内定义方法 | Parser, CodeGen |

### 较高难度（⭐⭐⭐）

| 特性 | 说明 | 涉及文件 |
|------|------|----------|
| 类继承 `class B extends A` | 继承链 + VTable 扩展 + `parent::` | Parser, CodeGen, types.h |
| 接口 `interface` / `implements` | 接口定义 + 实现检查 | Parser, CodeGen |
| 抽象类 `abstract class` | 抽象方法 + 实例化禁止 | Parser, CodeGen |
| `t_string` SSO 优化 | 24 字节内联缓冲区，字符串拼接 2-3x 加速 | types.h, runtime.h |
| `array_filter/array_map/array_reduce` | 闭包作为数组操作参数 | CodeGen, builtin.h |
| `sort/usort` 系列 | 排序函数 | CodeGen, builtin.h |

