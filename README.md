# TinyPHP

> PHP → C 转译编译器，将 PHP 子集转为安全 C 代码，由编译器编译为原生产物（可执行文件、静态库、动态库等）。

## 快速开始

```bash
# 编译单文件
php tphp.php test/var/var.php

# 编译多文件
php tphp.php main.php demo.php lib/helper.php

# 扫描当前目录所有 .php 编译
php tphp.php .

# 指定输出
php tphp.php main.php -o app.exe

# 指定编译器（默认内置 TCC）
php tphp.php main.php -cc gcc
php tphp.php main.php -cc clang
```

编译后直接运行生成的产物即可（Windows 为 `.exe`，Linux/macOS 为无后缀可执行文件）。

## 编译流水线

```
PHP 源码 → Lexer → Token[] → Parser → AST → CodeGenerator → .c → 编译器 → 产物
```

- `src/Lexer.php` — 词法分析，逐字符扫描生成 Token 流
- `src/Parser.php` — 递归下降解析，构建 AST
- `src/CodeGenerator.php` — 访问者模式生成 C 代码
- `tcc/win32/` — 内置 TCC 0.9.27（需 MSYS2 MinGW64 编译）

## 支持的语言特性

### 类型系统

| PHP | C |
|-----|---|
| `int` | `int64_t` |
| `float` | `double` |
| `string` | `struct { char *data; int length; }` |
| `bool` | `bool` |
| `null` | `void *` |
| `array` | `t_array *`（有序映射，int/string 键，支持嵌套） |
| `callable` | `t_callback { void *func; void *env; }` |

### 语法支持

```php
class Main {
    public function main(): void {
        // 变量
        $a = 10;
        $b = "hello";
        $c = true;
        $d = 1.01;
        $e = null;

        // 运算
        $z = $a + 5;

        // 字符串拼接
        $s = $a . " " . $b;          // "10 hello"
        $s2 = "hello $d\n";          // 双引号插值
        $s3 = "hello {$d}\n";        // 花括号插值

        // 数组字面量
        $arr = [1, 2, 3];
        $nested = [10, "str", true, [4, 5]];

        // 数组访问
        $x = $arr[0];
        count($arr);                  // 3

        // 输出
        echo "hello\n";

        // 调试（完整类型输出）
        var_dump($a);                 // int(10)
        var_dump($arr);               // array(3) { [0]=> int(1) ... }
        var_dump($nested);            // 递归嵌套输出

        // 对象
        $d = new Demo();
        $d->hello();

        // 匿名函数 / 闭包 + 调用
        $fn = function (): int { return 10; };
        var_dump($fn());              // int(10)
        $fn2 = function (int $x, int $y): int { return $x + $y; };
        var_dump($fn2(1, 2));         // int(3)
    }
}
```

### 类型强制转换

| 转换 | 示例 | 规则 |
|------|------|------|
| `(string)` | `(string)123` → `"123"` | int/float→数字串，bool→"1"/""，null→"" |
| `(int)` | `(int)"123abc"` → `123` | 字符串提取前导数字，float 截断 |
| `(float)` | `(float)"1.2e3"` → `1200` | 支持科学计数法 |
| `(bool)` | `(bool)"0"` → `false` | ""/"0"/0/0.0/null/[]→false，其余→true |
| `(array)` | `(array)123` → `[123]` | 标量→单元素数组，null→空数组 |

`(string)` 转换数组/对象时编译报错；`(int)`/`(float)` 转换对象时编译报错。

### 多文件 & 命名空间

```php
// main.php — 入口（必须有全局 class Main）
use Demo\Demo;
use function Demo\myFunc;

class Main { ... }

// demo.php — 命名空间
namespace Demo;
class Demo { ... }
function myFunc(): void { ... }

// other.php — 同名命名空间跨文件扩展
namespace Demo;
function myFunc2(): void { ... }
```

支持 `use Foo\Bar`、`use Foo\Bar as Alias`、`use Foo\{A, B, function F}` 组合导入。

### 不支持的

- `if` / `else` / `while` / `for` 等控制流
- `$a['key']` 字符串键数组赋值、多维数组赋值
- `use` 闭包变量捕获
- 游离代码（不在 class/function 内）
- 任何形式的 `include` / `require`

## C 运行时 (`include/`)

| 文件 | 内容 |
|------|------|
| `types.h` | 类型系统：`t_int`, `t_float`, `t_string`, `t_bool`, `t_var`, `t_array`, `t_object`, `t_callback` |
| `val.h` | 便捷宏：`VAR_INT`, `VAR_FLOAT`, `VAR_BOOL`, `VAR_STRING`, `VAR_ARRAY`, `VAR_CALLBACK`, `VAR_NULL`, `STR_LIT` |
| `array.h` | PHP 风格数组：`create`, `push`, `set_str/int`, `get_str/int`, `index`, `item_int/float/str/bool`, `count`, `free` |
| `function.h` | 运行时：`tphp_init`, `tphp_echo`, `tphp_var_dump`, `tphp_str_concat`, `tphp_parse_int/float`, `tphp_str_from_*`, `tphp_object_free` |

## 输出结构

```
执行目录/
├── main.exe        ← 编译产物
└── build/
    └── main.c      ← 中间 C 代码（每次编译前清空）
```

所有 C 标识符统一加 `tphp_` 前缀，避免与标准库冲突。

## CLI 选项

| 选项 | 说明 |
|------|------|
| `-o <output>` | 输出文件路径（默认执行目录下以入口文件名命名） |
| `-cc <compiler>` | 指定 C 编译器（默认内置 TCC） |
| `-h, --help` | 显示帮助 |

## 构建 TCC

```bash
# Windows (MSYS2 MinGW64)
cd tcc/win32 && cmd /c build-tcc.bat

# Linux / macOS
cd tcc && ./configure && make
```

## 许可证

MIT
