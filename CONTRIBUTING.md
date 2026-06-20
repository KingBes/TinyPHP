# TinyPHP 开发指南

> 面向 AI 及开发者：项目架构、扩展点、代码生成模式、安全规范。

---

## 1. 架构总览

```
tphp.php                        入口 CLI（参数解析、文件收集、AST 合并、调用编译器）
tphp / tphp.cmd                  Linux/macOS / Windows 快捷入口
build.sh / build.cmd             TCC 构建脚本

  └─ src/
       ├── TokenType.php         Token 枚举（PHP 8.1 enum, ~75 token）
       ├── Token.php             Token 值对象
       ├── AST/Node.php          AST 节点 + Visitor 接口
       ├── Lexer.php             词法分析 → Token[]
       ├── Parser.php            递归下降解析 → AST
       ├── CodeGenerator.php     访问者模式遍历 AST → C 代码
       └── Compiler.php          独立 API

include/                         C 运行时头文件（静态 inline 库）
  ├── common.h                   总入口
  ├── types.h                    类型定义
  ├── val.h                      便捷宏 (VAR_INT, STR_LIT, …)
  ├── array.h                    PHP 数组（引用计数 + 对象池）
  ├── runtime.h                  内部辅助（字符串、资源追踪、error）
  ├── builtin.h                  公开内置（echo, var_dump, exit, …）
  └── os/
      └── times.h                系统函数（time, date, sleep, usleep, hrtime）
```

> TCC 不在仓库中——通过 `build.sh`/`build.cmd` 从 `https://repo.or.cz/tinycc.git` (mob 分支) clone 并编译。

---

## 2. 编译流水线详解

### 2.1 Lexer（词法分析）

**文件**: `src/Lexer.php`

**令牌扫描顺序**（优先级从高到低）：
1. 空白 / 换行（CRLF → 一次 line++）
2. `/` → `//` 行注释 / `/* */` 块注释 / `/=` 复合赋值 / `/` 除号
3. **三字符运算符**（`<=>` 必须在 `<=` 之前）
4. **多字符运算符**（`->` `=>` `==` `!=` `<=` `>=` `**` `&&` `||` `++` `--` `+=` `-=` `*=` `/=` `.=` `<<` `>>` `??`）
5. **单字符运算符**（`+` `-` `*` `<` `>` `!` `&` `|` `^` `~` `?`）
6. 字符串（`"` 双引号含插值 / `'` 单引号 / heredoc / nowdoc）
7. `\` → 命名空间分隔符 `NS_SEP`
8. **`::` 双冒号**（必须在单字符 `:` 之前）
9. 单字符 `:` `;` `,` `=` `.` `(` `)` `{` `}` `[` `]`
10. `$` → 变量名
11. 数字 → int / float
12. 标识符 / 关键字

**字符串插值**：双引号和 heredoc 内 `{$var->prop}` 支持链式属性访问（`->` 重复解析）。

### 2.2 Parser（语法分析）

**运算符优先级**（从低到高）：

`??` < `?:` < `\|\|` < `&&` < `==` `!=` `<=>` < `<` `>` `<=` `>=` < `<<` `>>` < `+` `-` `.` < `*` `/` `%` < `**`（右结合）< `!` `-` `++` `--` `~`（一元）< 后缀链

### 2.3 AST（抽象语法树）

**文件**: `src/AST/Node.php`

节点层次（重点）：

```
StmtNode（抽象）
├── EchoStmtNode          # echo
├── ReturnStmtNode        # return
├── AssignStmtNode        # $var = expr
├── AssignPropStmtNode    # $this->prop = expr
├── AssignArrayStmtNode   # $arr[$i] = expr
├── ExprStmtNode          # expr;
├── IfStmtNode            # if / elseif / else
├── WhileStmtNode / DoWhileStmtNode / ForStmtNode / ForeachStmtNode
├── SwitchStmtNode / MatchExpr / BreakStmtNode / ContinueStmtNode
└── GotoStmtNode

ExprNode（抽象，含 line/column）
├── StringLiteralExpr / IntLiteralExpr / FloatLiteralExpr
├── BoolLiteralExpr / NullLiteralExpr
├── VariableExpr / UnaryExpr / PostfixExpr
├── BinaryExpr            # 含 ** （幂）和 <=> （太空船）
├── CompoundAssignExpr    # += -= *= /= .=
├── CallExpr              # 函数 / 方法调用
├── CastExpr / NewExpr / ArrayLiteralExpr
├── ArrayAccessExpr / PropertyAccessExpr
├── EnumAccessExpr / ClosureExpr
├── TernaryExpr / NullCoalesceExpr
└── MatchArm / MatchExpr
```

**访问者模式**：每个 `ASTNode::accept(ASTVisitor)` → visitor 对应方法。`CodeGenerator` 是唯一的 `ASTVisitor` 实现。

### 2.4 CodeGenerator（代码生成）

**关键内部状态**：

| 属性 | 说明 |
|------|------|
| `$this->className` | 当前类的 C 名 |
| `$this->varTypes` | `varName → C 类型` 映射 |
| `$this->declaredVars` | 已声明变量集合 |
| `$this->scopeObjects` | 作用域对象列表（方法结尾自动析构） |
| `$this->classPropTypes` | 类属性类型表 |
| `$this->classMethodRetTypes` | 类方法返回类型表 |
| `$this->enumBackingTypes` | 枚举 backing 类型 |
| `$this->closureSigs` | 闭包签名 |
| `$this->phpFile` | 当前 PHP 源文件路径（error 用） |
| `$this->indent` | 当前缩进级别 |

**命名规则**：

| PHP | C | 前缀 |
|-----|---|------|
| `class Main` | `tphp_class_Main` | `tphp_class_` |
| `namespace Demo; class Demo` | `tphp_class_Demo_Demo` | `tphp_class_` |
| `new Demo()` | `new_tphp_class_Demo()` | `new_` |
| `function foo()` | `tphp_fn_foo` | `tphp_fn_` |
| `enum Color: string` | `tphp_enum_Color` | `tphp_enum_` |
| `Color::RED` | `&_e_Color_RED` | `_e_` |
| `$a . $b` | `tphp_rt_str_concat(a, b)` | `tphp_rt_` |
| `const APP_NAME` | `#define TPHP_CONST_APP_NAME` | `TPHP_CONST_` |

### 2.5 tphp.php（入口 CLI）

**两阶段解析**：
1. 先解析辅助文件（非 Main），收集枚举名/类名
2. 再解析 Main 入口文件（此时 `setKnownEnums` 已注入所有枚举名）

**Main 类合并**：扫描所有类中名为 `Main` 且全局命名空间的类作为入口，其余为辅助类。

---

## 3. C 运行时

### 3.1 头文件职责

| 文件 | 前缀 | 职责 |
|------|------|------|
| `common.h` | — | 总入口 |
| `types.h` | — | 类型系统 |
| `val.h` | `VAR_*` `STR_LIT` | 便捷宏 |
| `array.h` | `tphp_fn_arr_` | PHP 数组（对象池优化） |
| `runtime.h` | `tphp_rt_` | 内部辅助、资源追踪、error |
| `builtin.h` | `tphp_fn_` | 公开内置 |
| `os/times.h` | `tphp_fn_` | 系统函数 |

### 3.2 资源追踪 & error()

`error($msg)` 调用 `tphp_fn_error()`：先遍历全局资源链表释放所有对象/数组/字符串，再打印错误消息（含 PHP 源文件和行号）并 `exit(1)`。

`tphp_rt_register(ptr, type)` 在 `new` 和数组创建时自动注册，`unset` 时注销。

### 3.3 数组对象池

`t_array` 使用 256 槽对象池，创建时优先复用已释放的 `t_array` 结构体（`memset(0)` 后 `refcount=1`），避免频繁 `malloc/free`。

---

## 4. 扩展指南

### 添加新运算符

1. `TokenType`：添加枚举值
2. `Lexer`：多字符加入 `$multiOps`，三字符特殊处理（如 `<=>`）
3. `Parser`：在对应优先级方法中添加匹配
4. `AST/Node.php`：添加 `XxxExpr extends ExprNode` + Visitor 方法（简单运算符可用 `BinaryExpr`）
5. `CodeGenerator`：`visitBinary()` 中处理 + `inferType()` 返回类型

### 添加内置函数

1. `TokenType` + `Lexer::$keywords` 添加关键词
2. `Parser`：`parsePrimary()` 标识符检查 + 跳过命名空间解析
3. `CodeGenerator::visitCall()`：添加处理分支
4. `CodeGenerator::inferCallReturnType()`：添加返回类型
5. C 运行时：在 `include/` 下添加实现，并在 `common.h` 中 `#include`
6. 测试：添加测试文件到 `test/var/`

### 添加系统函数

参照 `os/times.h` 模式：单独头文件 + `common.h` 引用。注意跨平台兼容（`#ifdef _WIN32`）。

### 添加新语句

1. `TokenType` + `Lexer`：添加关键字
2. `AST/Node.php`：添加 `XxxStmtNode extends StmtNode` + Visitor 方法
3. `Parser::parseStmt()`：添加匹配分发 + 实现 `parseXxxStmt()`
4. `CodeGenerator`：实现 `visitXxxStmt()`

---

## 5. 安全编码规范

### 内存安全

- **error() 全局清理**：退出前遍历资源链释放所有对象/数组/字符串
- **对象池复用**：`t_array` 释放时回收，创建时 `memset(0)` 防止脏数据
- **字符串深拷贝**：对象属性赋值时先 `tphp_rt_str_free` 再 `tphp_rt_str_dup`
- **析构自动释放**：类 `__destruct` 中自动释放所有 `t_string` 属性
- **数组引用计数**：嵌套数组 `push/set` 自动 `retain`
- **零堆分配函数**：`time()` `date()` `sleep()` `usleep()` `hrtime()` 全静态缓冲区

### 命名防冲突

| 前缀 | 用途 |
|------|------|
| `tphp_class_` | 用户类 |
| `tphp_fn_` | 函数（内置 + 用户） |
| `tphp_rt_` | 运行时内部 |
| `tphp_enum_` | 枚举结构体 |
| `_e_` | 枚举 static 实例 |
| `_cap_` | 闭包捕获 struct |
| `TPHP_CONST_` | 常量宏 |
| `_arr_` / `_tmp_` | CodeGenerator 临时变量 |

### 代码质量

- **空指针检查**：方法入口 `if (self == NULL) return;`
- **数组创建**：`if (_arr != NULL) { ... }` 包裹
- **类型不可变**：变量一旦赋值类型固定
- **TCC 兼容**：避免 C99 特性 TCC 不支持（如 `for` 变量作用域提升到函数级）

---

## 6. 文件索引

| 文件 | 行数~ | 核心职责 |
|------|------|---------|
| `tphp.php` | ~310 | CLI 入口、两阶段解析、多文件合并、编译器调用 |
| `src/TokenType.php` | ~125 | Token 枚举 (~75 token) |
| `src/Token.php` | ~20 | Token 值对象 |
| `src/AST/Node.php` | ~820 | AST 节点 + Visitor 接口 |
| `src/Lexer.php` | ~680 | 词法分析（链式属性插值、heredoc、运算符） |
| `src/Parser.php` | ~1380 | 递归下降解析（**/<=>/混合/match/enum/closure/goto） |
| `src/CodeGenerator.php` | ~2150 | C 代码生成（内置函数/time系列/error/数组赋值/资源追踪） |
| `include/types.h` | ~115 | C 类型系统 |
| `include/val.h` | ~45 | 便捷宏 |
| `include/array.h` | ~290 | PHP 数组（对象池优化） |
| `include/runtime.h` | ~290 | 运行时（资源追踪、error、字符串、幂运算） |
| `include/builtin.h` | ~150 | 公开内置 |
| `include/os/times.h` | ~95 | 系统函数（跨平台） |
| `include/common.h` | ~15 | 总入口 |
| `test/var/` | 31 文件 | 测试用例（运算符/时间/error/数组性能） |
