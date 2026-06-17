# TinyPHP 开发指南

> 面向 AI 及开发者：项目架构、扩展点、代码生成模式。

---

## 1. 架构总览

```
tphp.php                        入口 CLI（参数解析、文件收集、AST 合并、调用编译器）
  └─ src/
       ├── TokenType.php         Token 枚举
       ├── Token.php             Token 值对象 (type, lexeme, line, column, literal)
       ├── AST/Node.php          AST 节点 + Visitor 接口
       ├── Lexer.php             词法分析 → Token[]
       ├── Parser.php            递归下降解析 → AST
       ├── CodeGenerator.php     访问者模式遍历 AST → C 代码
       └── Compiler.php          独立 API（逻辑在 tphp.php）

include/                         C 运行时头文件
  ├── common.h                   总入口
  ├── types.h                    类型定义 (t_int, t_string, t_array, t_var, …)
  ├── val.h                      便捷宏 (VAR_INT, STR_LIT, …)
  ├── array.h                    PHP 风格数组实现
  └── function.h                 运行时函数 (echo, var_dump, 转换, 内存管理)

tcc/                            内置 TCC 编译器源码
  └── win32/build-tcc.bat       Windows 构建脚本
```

---

## 2. 编译流水线详解

### 2.1 Lexer（词法分析）

**文件**: `src/Lexer.php`

```php
$lexer  = new Lexer($source);
$tokens = $lexer->tokenize();   // → Token[]
```

**换行处理**：
- `\r\n` (CRLF) → 一次 `line++`
- `\n` (LF) → 一次 `line++`
- `\r` 单独 → 一次 `line++`
- 空格/制表符不改变行列

**如何扩展**：
- 新增关键字 → `$keywords` 数组 + `TokenType` 枚举
- 新增运算符/符号 → `$singleChars` 数组或手动处理（如 `.` 运算符）
- 块注释 `/* */` → `skipBlockComment()`
- 行注释 `//` → `skipLineComment()`
- 字符串插值 → `scanString()` 中检测 `$var`/`{$var}`，自动插入 DOT token

### 2.2 Parser（语法分析）

**文件**: `src/Parser.php`

递归下降解析器。当前语法：

```
program      → PHP_OPEN namespace_decl? use_decl* decl* EOF
decl         → class_decl | function_decl
class_decl   → CLASS_KW IDENTIFIER LBRACE method* RBRACE
method       → visibility FUNCTION name LPAREN params? RPAREN COLON type LBRACE stmt* RBRACE
function     → FUNCTION IDENTIFIER LPAREN params? RPAREN COLON type LBRACE stmt* RBRACE
stmt         → echo_stmt | return_stmt | assign_stmt | expr_stmt
assign_stmt  → IDENTIFIER EQUALS expr SEMICOLON
expr         → additive ((PLUS|MINUS|DOT) additive)*
additive     → multiplicative ((PLUS|MINUS) multiplicative)*
multiplicative → primary ((STAR|SLASH) primary)*
primary      → literal | variable | call | new | cast | closure | array_literal | unary | array_access
```

**如何添加新语句**（以 `if` 为例）：

1. `TokenType` 添加 `IF_KW`, `ELSE_KW`
2. `Lexer::$keywords` 添加 `'if' => TokenType::IF_KW`
3. `AST/Node.php` 添加 `IfStmtNode extends StmtNode`
4. `ASTVisitor` 添加 `visitIfStmt(IfStmtNode): string`
5. `Parser::parseStmt()` 添加匹配分发
6. `CodeGenerator` 添加 `visitIfStmt()`

### 2.3 AST（抽象语法树）

**文件**: `src/AST/Node.php`

节点层次（所有 `ExprNode` 携带 `line`/`column` 用于错误定位）：

```
ASTNode（抽象）
├── ProgramNode          根节点 (mainClass? + extraClasses + functions)
├── ClassNode            类定义 (name, methods, namespace)
├── FunctionNode         独立函数 (name, params, returnType, body, namespace)
├── MethodNode           方法 (name, visibility, params, returnType, body)
├── ParamNode            参数 (type, name)
├── StmtNode（抽象）
│   ├── EchoStmtNode
│   ├── ReturnStmtNode
│   ├── AssignStmtNode
│   └── ExprStmtNode
└── ExprNode（抽象，含 line/column）
    ├── StringLiteralExpr / IntLiteralExpr / FloatLiteralExpr
    ├── BoolLiteralExpr / NullLiteralExpr
    ├── VariableExpr
    ├── UnaryExpr           一元运算（如负号）
    ├── BinaryExpr          二元运算 (+ - * / .)
    ├── CallExpr            函数/方法调用 (callee? + name + args)
    ├── CastExpr            类型转换 (castType + expr)
    ├── NewExpr             new ClassName(args)
    ├── ArrayLiteralExpr    [1, 2, 3]
    ├── ArrayAccessExpr     $arr[0]
    └── ClosureExpr         匿名函数
```

**访问者模式**：每个 `ASTNode::accept(ASTVisitor)` → visitor 对应方法。`CodeGenerator` 是唯一的 `ASTVisitor` 实现。

### 2.4 CodeGenerator（代码生成）

**文件**: `src/CodeGenerator.php`

**生成结构**（`visitProgram` 输出顺序）：

```c
#include "common.h"

// Phase 1 — 前置声明
typedef struct { t_object _base; } tphp_Main;
void tphp_Main_main(tphp_Main* self);
tphp_Main* new_tphp_Main(...);
static void tphp_myFn();

// Phase 2 — 实现
void tphp_Main_main(tphp_Main* self) { ... }
tphp_Main* new_tphp_Main(...) { ... }
static void tphp_myFn() { ... }

// 闭包实现（文件作用域）
t_int _closure_1() { ... }

// C 入口
int main(int argc, char* argv[]) { tphp_init(); ... }
```

**关键内部状态**：

| 属性 | 说明 |
|------|------|
| `$this->className` | 当前类的 C 名（如 `tphp_Main`） |
| `$this->varTypes` | `varName → C 类型` 映射 |
| `$this->declaredVars` | 已声明变量集合 |
| `$this->scopeObjects` | 作用域内对象列表（结尾自动析构） |
| `$this->closureImpls` | 闭包实现数组 |
| `$this->indent` | 当前缩进级别 |

**命名规则**：

| PHP | C |
|-----|---|
| `class Main` | `tphp_Main` |
| `Main::hello()` | `tphp_Main_hello` |
| `namespace Demo; class Demo` | `tphp_Demo_Demo` |
| `new Demo()` | `new_tphp_Demo_Demo(...)` |
| `function allfn()` | `tphp_allfn` |
| `namespace Demo; function fn()` | `tphp_Demo_fn` |
| `var_dump($x)` | `tphp_var_dump(VAR_INT(x))` |
| `$f = function(): int {…}` | `({ t_int _closure_1(); (t_callback){…} })` |
| `$h()` | `((t_int(*)(void))h.func)()` |
| `(int)$x` | `tphp_parse_int(x)` 或 `(t_int)(x)` |
| `$a . $b` | `tphp_str_concat(a, b)` |
| `$arr[0]` | `tphp_arr_item_int(arr, 0)` |

**类型转换代码生成套路**：
- `visitCast` 分发到 `castToInt` / `castToFloat` / `castToBool` / `castToStr` / `castToArray`
- `castToStr` 有 `$strict` 参数：true 时数组/对象报错，false 时静默转 "Array"/"Object"
- `wrapVar`（var_dump 用）根据表达式类型选择 `VAR_*` 宏

### 2.5 tphp.php（入口 CLI）

1. 解析参数（`-o`, `-cc`, `.` 目录扫描）
2. 收集 `.php` 文件（递归跳过 `build/`）
3. 依次 Lexer → Parser 解析每个文件
4. 合并 AST（全局 `class Main` 为入口，其余归入 extraClasses）
5. 清理 `build/` → CodeGenerator 生成 `.c`
6. 调用编译器 → 产物

---

## 3. C 运行时

### 3.1 类型系统（`types.h`）

```c
typedef enum { TYPE_NULL=0, TYPE_INT=1, TYPE_FLOAT=2, TYPE_BOOL=3,
               TYPE_STRING=4, TYPE_ARRAY=5, TYPE_OBJECT=6, TYPE_CALLBACK=7 } type_t;

typedef int64_t t_int;      typedef double  t_float;      typedef bool t_bool;
typedef struct { char *data; int length; } t_string;
typedef struct { void *func; void *env; } t_callback;

struct _t_var  { type_t type; t_value value; };
struct _t_array { t_entry *entries; int length, capacity, refcount; };
typedef struct { const ClassVTable *vtable; int refcount; } t_object;
```

### 3.2 数组 API（`array.h`）

```c
t_array* tphp_arr_create(void);
void     tphp_arr_push(t_array *a, t_var val);
void     tphp_arr_set_str(t_array *a, t_string key, t_var val);
void     tphp_arr_set_int(t_array *a, t_int key, t_var val);
t_var*   tphp_arr_index(t_array *a, int idx);       // 按位置取 t_var*
t_int    tphp_arr_item_int(t_array *a, int idx);    // 按位置取 t_int
t_float  tphp_arr_item_float(t_array *a, int idx);
t_string tphp_arr_item_str(t_array *a, int idx);
t_bool   tphp_arr_item_bool(t_array *a, int idx);
int      tphp_arr_count(t_array *a);
void     tphp_arr_free(t_array *a);
```

### 3.3 运行时函数（`function.h`）

```c
void  tphp_init(void);                        // Windows 下设置控制台 UTF-8
void  tphp_echo(t_string s);                  // 二进制安全输出
void  tphp_var_dump(t_var v);                 // 递归类型打印
t_string tphp_str_concat(t_string a, t_string b);  // 字符串拼接（堆分配）
t_int    tphp_parse_int(t_string s);          // 字符串→整数
t_float  tphp_parse_float(t_string s);        // 字符串→浮点数（strtod）
t_string tphp_str_from_int/float/bool(t_* v); // 基础类型→字符串
void     tphp_object_free(t_object *obj);     // 引用计数析构
void     tphp_err(const char *msg);           // 运行时致命错误
```

### 3.4 类型包装宏（`val.h`）

```c
VAR_INT(10)       // → (t_var){.type=TYPE_INT, .value._int=10}
VAR_FLOAT(3.14)   VAR_BOOL(true)    VAR_STRING(s)
VAR_ARRAY(a)      VAR_CALLBACK(c)   VAR_NULL()
STR_LIT("hello")  // → (t_string){.data="hello", .length=5}  编译期计算
```

---

## 4. 扩展指南

### 添加新表达式

1. `TokenType` + `Lexer::$keywords`：添加 token
2. `AST/Node.php`：添加 `XxxExpr extends ExprNode`
3. `ASTVisitor`：添加 `visitXxx`
4. `Parser::parsePrimary()`：添加匹配
5. `CodeGenerator`：实现 `visitXxx()`

### 添加类型转换（如 `(bool)`, `(array)`）

1. `CodeGenerator::castToXxx(ExprNode)`：按字面量/变量类型推导生成转换代码
2. `visitCast`：分发到对应方法
3. C 运行时：如需解析函数（如 `tphp_parse_float`）添加到 `function.h`
4. `wrapVar`：添加 `CastExpr` 分支选择正确的 `VAR_*` 宏

### 添加内置函数（如 `count`）

1. `TokenType` + `Lexer::$keywords` 添加关键词
2. `Parser`：函数调用路径不解析命名空间
3. `CodeGenerator::visitCall()`：特殊处理生成对应 C 函数调用

---

## 5. 安全编码规范

- **所有 C 标识符**以 `tphp_` 开头，避免与标准库冲突
- **空指针检查**：方法入口 `if (self == NULL) return;`
- **数组创建**：`if (_arr != NULL) { ... }` 包裹
- **var_dump**：entry 的 key/value 空指针检查，字符串长度负数保护
- **str_concat**：分配前检查长度，失败返回 `{NULL, 0}`
- **编译器报错**：多文件拒绝游离代码，类型转换报语法错误（含行列）

---

## 6. 文件索引

| 文件 | 行数 | 核心职责 |
|------|------|---------|
| `tphp.php` | ~230 | CLI 入口、多文件收集、AST 合并、编译器调用 |
| `src/TokenType.php` | ~67 | Token 枚举 |
| `src/Token.php` | ~20 | Token 值对象 |
| `src/AST/Node.php` | ~390 | 全部 AST 节点 + Visitor 接口 |
| `src/Lexer.php` | ~320 | 词法分析（换行/注释/插值/命名空间分隔） |
| `src/Parser.php` | ~530 | 递归下降解析（namespace/use/表达式/语句/数组访问） |
| `src/CodeGenerator.php` | ~800 | C 代码生成（类型推导/闭包/命名空间/类型转换） |
| `include/types.h` | ~105 | C 类型系统 |
| `include/val.h` | ~36 | 便捷宏 |
| `include/array.h` | ~265 | PHP 数组实现（含按位置类型取值） |
| `include/function.h` | ~225 | 运行时函数（echo/var_dump/拼接/转换/初始化） |
| `include/common.h` | ~10 | 总入口 |
