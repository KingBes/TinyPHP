# TinyPHP 语法参考

> 基于 PHP 8.5.7 `zend_language_parser.y`，标注 TinyPHP 当前支持程度。
> ✅ 已支持 | ⬜ 待实现 | ❌ AOT 不可行

---

## 1. 顶层声明（Top-level）

```
program:
    program statement_top
  | %empty

statement_top:
    namespace_decl
  | use_decl
  | const_decl
  | function_decl              ✅
  | class_decl                 ✅
  | enum_decl                  ✅
  | #include 指令               ✅ TinyPHP 扩展
  | #flag 指令                  ✅ TinyPHP 扩展
  | #callback 指令              ✅ TinyPHP 扩展
  | interface_decl             ✅
  | trait_decl                 ✅
  | abstract_class_decl        ✅
```

---

## 2. 命名空间 / Use

```
namespace_decl:
    'namespace' name ';'                    ✅
  | 'namespace' name '{' statement_top* '}' ✅ (多文件模式不支持)

name:
    IDENTIFIER                              ✅
  | name '\' IDENTIFIER                    ✅

use_decl:
    'use' name ';'                          ✅ (类导入)
  | 'use' 'function' name ';'              ✅
  | 'use' name 'as' IDENTIFIER ';'         ✅
  | 'use' group_use                        ✅ (use A\{B, C})
```

---

## 3. 类声明

```
class_decl:
    modifier* 'class' IDENTIFIER extends? implements? '{' member* '}'   ✅ 部分

modifier:
    'abstract'      ✅
  | 'final'         ✅ (仅修饰符，无运行时检查)
  | 'readonly'      ✅ (仅修饰符)

extends:
    'extends' name   ✅

implements:
    'implements' name (',' name)*   ✅

member:
    property_decl         ✅
  | method_decl           ✅
  | const_decl            ✅
  | 'use' trait_name      ✅
  | enum_case             ✅ (enum only)
```

### 3.1 属性

```
property_decl:
    type? IDENTIFIER '=' expr ';'                         ✅ (默认值有限)
  | visibility 'static'? 'readonly'? type IDENTIFIER ';'  ✅
  | visibility 'static'? 'readonly'? type IDENTIFIER '=' expr ';'  ✅

visibility:
    'public'    ✅
  | 'private'   ✅

type:
    'int'       ✅ → int64_t
  | 'float'     ✅ → double
  | 'string'    ✅ → t_string
  | 'bool'      ✅ → bool
  | 'array'     ✅ → t_array*
  | 'void'      ✅
  | 'never'     ✅
  | 'mixed'     ✅ → t_var
  | 'callable'  ✅ → t_callback
  | name        ✅ (类/枚举类型)
  | '?' type    ⬜ (nullable)
  | type '|' type   ⬜ (union types)
  | '(' type (',' type)+ ')'   ⬜ (intersection types)
```

### 3.2 方法

```
method_decl:
    visibility 'function' IDENTIFIER '(' params ')' return_type? body   ✅
  | visibility 'static' 'function' IDENTIFIER '(' params ')' return_type? body  ✅ (部分)

return_type:
    ':' type   ✅
  | %empty    ✅ (默认 void)

特殊方法:
    '__construct'  ✅ (支持属性提升: 'public' type '$var' 语法)
    '__destruct'   ✅ (禁止写返回类型)
```

### 3.3 类常量

```
const_decl:
    'const' IDENTIFIER '=' expr ';'       ✅ (int/float/string/bool/array)
  | visibility 'const' type IDENTIFIER '=' expr ';'  ⬜ (typed constants)
```

---

## 4. 枚举

```
enum_decl:
    'enum' IDENTIFIER ':' backing_type '{' case* '}'   ✅

backing_type:
    'int'      ✅
  | 'string'   ✅

enum_case:
    'case' IDENTIFIER '=' expr ';'   ✅
```

---

## 5. 函数

```
function_decl:
    'function' IDENTIFIER '(' params ')' return_type? body   ✅

closure:
    'function' '(' params ')' use_vars? return_type? body    ✅
  | 'fn' '(' params ')' '=>' expr                            ✅ (箭头函数)

use_vars:
    'use' '(' var_list ')'   ✅
```

---

## 6. 参数

```
params:
    param (',' param)* ','?   ✅ (尾部逗号支持)

param:
    type IDENTIFIER '=' expr  ⬌ (默认值仅部分类型支持)
  | type IDENTIFIER            ✅
  | IDENTIFIER                 ✅ (无类型，箭头函数)
  | 'public' type '$' IDENTIFIER   ✅ (构造器属性提升)
  | 'private' type '$' IDENTIFIER  ✅ (构造器属性提升)
```

---

## 7. 语句

```
statement:
    expr ';'                   ✅
  | echo_stmt                  ✅
  | if_stmt                    ✅
  | while_stmt                 ✅
  | do_while_stmt              ✅
  | for_stmt                   ✅
  | foreach_stmt               ✅
  | switch_stmt                ✅
  | match_stmt                 ✅ (多条件支持)
  | return_stmt                ✅
  | break_stmt                 ✅
  | continue_stmt              ✅
  | goto_stmt                  ✅
  | try_stmt                   ✅ (COS setjmp/longjmp)
  | throw_stmt                 ✅ (Exception 类)
  | assign_stmt                ✅
  | compound_assign            ✅ (+= -= *= /= .=)
  | list_destructure           ✅ (含键名 "key"=>$var)
  | unset_stmt                 ✅
  | block                      ✅
```

### 7.1 具体语法

```
echo_stmt:
    'echo' expr (',' expr)* ';'   ✅

if_stmt:
    'if' '(' expr ')' body elseif* else?   ✅

while_stmt:
    'while' '(' expr ')' body   ✅

do_while_stmt:
    'do' body 'while' '(' expr ')' ';'   ✅

for_stmt:
    'for' '(' for_exprs ';' for_exprs ';' for_exprs ')' body   ✅

foreach_stmt:
    'foreach' '(' expr 'as' value ')' body                            ✅
  | 'foreach' '(' expr 'as' key '=>' value ')' body                  ✅
  | 'foreach' '(' expr 'as' '&' value ')' body                       ✅

switch_stmt:
    'switch' '(' expr ')' '{' case* default? '}'   ✅

match_stmt:
    'match' '(' expr ')' '{' arm (',' arm)* ','? '}'   ✅
arm:
    expr (',' expr)* '=>' expr   ✅ (多条件)

return_stmt:
    'return' expr? ';'   ✅

break_stmt:
    'break' INT_LIT? ';'   ✅

continue_stmt:
    'continue' INT_LIT? ';'   ✅

goto_stmt:
    'goto' IDENTIFIER ';'   ✅
    IDENTIFIER ':'           ✅ (标签)

try_stmt:
    'try' '{' statement* '}' catch* finally?   ✅

throw_stmt:
    'throw' expr ';'   ✅
```

---

## 8. 表达式

### 8.1 运算符优先级（从低到高）

```
expr:
    yield_expr       ❌
  | ternary_expr     ✅
  | logical_or       ✅
  | logical_and      ✅
  | bitwise_or       ✅
  | bitwise_xor      ✅
  | bitwise_and      ✅
  | equality         ✅
  | comparison       ✅
  | spaceship        ✅
  | bitwise_shift    ✅
  | additive         ✅
  | multiplicative   ✅
  | power            ✅
  | instanceof       ⬜
  | prefix           ✅
  | postfix          ✅
  | primary          ✅

ternary_expr:
    expr '?' expr ':' expr   ✅
  | expr '?:' expr           ✅
  | coalesce_expr            ✅

coalesce_expr:
    expr '??' expr   ✅

logical_or:      expr '||' expr     ✅
logical_and:     expr '&&' expr     ✅
bitwise_or:      expr '|' expr      ✅
bitwise_xor:     expr '^' expr      ✅
bitwise_and:     expr '&' expr      ✅

equality:
    expr '==' expr    ✅
  | expr '!=' expr    ✅
  | expr '===' expr   ✅
  | expr '!==' expr   ✅

comparison:
    expr '<' expr   ✅
  | expr '>' expr   ✅
  | expr '<=' expr  ✅
  | expr '>=' expr  ✅

spaceship:
    expr '<=>' expr   ✅

bitwise_shift:
    expr '<<' expr   ✅
  | expr '>>' expr   ✅

additive:
    expr '+' expr  ✅
  | expr '-' expr  ✅
  | expr '.' expr  ✅ (字符串拼接)

multiplicative:
    expr '*' expr   ✅
  | expr '/' expr   ✅
  | expr '%' expr   ✅

power:
    expr '**' expr   ✅

prefix:
    '!' expr      ✅
  | '~' expr      ✅
  | '+' expr      ✅
  | '-' expr      ✅
  | '++' var      ✅
  | '--' var      ✅

postfix:
    var '++'                    ✅
  | var '--'                    ✅
  | expr '->' IDENTIFIER        ✅
  | expr '?->' IDENTIFIER       ✅ (nullsafe)
  | expr '->' IDENTIFIER '(' args ')'     ✅
  | expr '?->' IDENTIFIER '(' args ')'    ✅ (nullsafe call)
  | expr '::' IDENTIFIER                  ✅ (self:: / Class::)
  | expr '::' IDENTIFIER '(' args ')'     ✅ (self::method / Class::method)
  | expr '[' expr ']'          ✅
  | expr '(' args ')'          ✅
```

### 8.2 基础表达式

```
primary:
    INT_LIT          ✅ → t_int(int64_t)
  | FLOAT_LIT        ✅ → t_float(double)
  | STRING_LIT       ✅ → t_string
  | TRUE_KW          ✅ → true
  | FALSE_KW         ✅ → false
  | NULL_KW          ✅ → null
  | MAGIC_LINE       ✅ (__LINE__)
  | MAGIC_FILE       ✅ (__FILE__)
  | MAGIC_DIR        ✅ (__DIR__)
  | DIR_SEP          ✅ (DIRECTORY_SEPARATOR)
  | IDENTIFIER       ✅ (变量 $var 或常量 CONST)
  | '(' expr ')'     ✅
  | '(' cast_type ')' expr          ✅ ((int)/(float)/(string)/(bool))
  | 'array' '(' args ')'            ✅
  | '[' args ']'                    ✅
  | 'new' name '(' args ')'         ✅
  | 'function' '(' params ')' body  ✅ (闭包)
  | 'fn' '(' params ')' '=>' expr   ✅ (箭头函数)
  | 'match' '(' expr ')' '{' arm* '}' ✅
  | 'list' '(' list_vars ')' '=' expr   ✅
  | '[' list_vars ']' '=' expr           ✅
```

---

## 9. 运算符映射表

| PHP 运算符 | Token | C 输出 | 状态 |
|---|---|---|---|
| `+ - * / %` | PLUS/MINUS/STAR/SLASH/PERCENT | `+ - * / %` | ✅ |
| `**` | STAR_STAR | `tphp_rt_pow_int/float` | ✅ |
| `.` | DOT | `tphp_rt_str_concat` | ✅ |
| `=` | EQUALS | `=` | ✅ |
| `+= -= *= /= .=` | PLUS_EQ etc | `+= -= *= /=` (str concat for `.=`) | ✅ |
| `== !=` | EQ/NE | `== !=` (str: `tphp_rt_str_eq/ne`) | ✅ |
| `=== !==` | IDENTICAL/NOT_IDENTICAL | `== !=` (AOT 类型固定) | ✅ |
| `< > <= >=` | LT/GT/LE/GE | `< > <= >=` | ✅ |
| `<=>` | SPACESHIP | `((x)>(y)?1:((x)<(y)?-1:0))` | ✅ |
| `&& \|\| !` | AND/OR/NOT | `&& \|\| !` | ✅ |
| `& \| ^ ~` | AMP/PIPE/CARET/TILDE | `& \| ^ ~` | ✅ |
| `<< >>` | SL/SR | `<< >>` | ✅ |
| `++ --` | INC/DEC | `++ --` | ✅ |
| `?:` | QUEST/COLON | `?:` | ✅ |
| `??` | COALESCE/QUEST_QUEST | `(x) ? (x) : (y)` | ✅ |
| `?->` | NULLSAFE_ARROW | `if (obj) obj->m()` 块 | ✅ |
| `(int)` | INT_CAST | `(t_int)` | ✅ |
| `(float)` | FLOAT_CAST | `(t_float)` | ✅ |
| `(string)` | STRING_CAST | 按类型转换 | ✅ |
| `(bool)` | BOOL_CAST | 按假值规则 | ✅ |

---

## 10. Magical / Special Tokens

```
__LINE__              ✅
__FILE__              ✅
__DIR__               ✅
DIRECTORY_SEPARATOR   ✅
$this                 ✅
self                  ✅
$GLOBALS              ❌
__CLASS__             ✅
__METHOD__            ✅
__FUNCTION__          ⬜
__TRAIT__             ❌
```

---

## 11. 预处理器指令（TinyPHP 扩展）

```
#directive:
    '#include' '"' file '"'   ✅ (生成 #include "file" 到 C 代码)
  | '#include' '<' file '>'   ✅ (系统头文件)
  | '#flag' compiler? platform? flags   ✅ (编译器/平台过滤标志)
  | '#callback' type IDENTIFIER '(' params ')'   ✅ (声明 C 回调签名)
```

---

## 12. C 互操作扩展（TinyPHP 扩展）

```
c_call:
    'C->' IDENTIFIER '(' args ')'   ✅ (直接 C 函数调用)

c_type_bridge:
    'c_int(' expr ')'     ✅ → int32_t
  | 'c_float(' expr ')'   ✅ → double
  | 'c_str(' expr ')'     ✅ → const char*
  | 'php_int(' expr ')'   ✅ → t_int
  | 'php_float(' expr ')' ✅ → t_float
  | 'php_str(' expr ')'   ✅ → t_string (深拷贝)

phpc_array:
    'phpc_arr_int(' expr ')'       ✅ → int32_t* (malloc)
  | 'phpc_arr_dbl(' expr ')'       ✅ → double* (malloc)
  | 'phpc_arr_str(' expr ')'       ✅ → char** (malloc)
  | 'phpc_new_arr_int(' p,p ')'    ✅ → t_array*
  | 'phpc_new_arr_dbl(' p,p ')'    ✅ → t_array*
  | 'phpc_new_arr_str(' p,p ')'    ✅ → t_array*
  | 'phpc_new_arr()'               ✅ → t_array*

phpc_object:
    'phpc_obj(' expr ')'                       ✅ → void*
  | 'phpc_new_obj(' ptr ',' cls ')'            ✅ → t_object*

phpc_callback:
    'phpc_fn(' cb ')'              ✅ → void*
  | 'phpc_env(' cb ')'             ✅ → void*
  | 'phpc_fn_i32(' cb ')'          ✅ → int32_t(*)(int32_t, void*)
  | 'phpc_thunk(' name ',' cb ')'  ✅ → 按 #callback 签名生成 thunk

phpc_memory:
    'phpc_free(' ptr ')'           ✅ → free(ptr)
  | 'phpc_free_str_arr(' p,p ')'   ✅
```

---

## 13. 与 PHP 8.5 的差异汇总

| 语法 | PHP 8.5 | TinyPHP | 备注 |
|---|---|---|---|
| `class` | ✅ | ✅ | — |
| `extends` | ✅ | ✅ | COS struct 嵌套 |
| `implements` | ✅ | ✅ | 编译期契约（interface） |
| `interface` | ✅ | ✅ | 纯抽象类，编译期检查 |
| `trait` | ✅ | ✅ | 编译期扁平化（use TraitName;） |
| `abstract class` | ✅ | ✅ | 禁止 new，抽象方法无体 |
| `final class` | ✅ | ✅ | — |
| `readonly class` | ✅ | ✅ | — |
| `enum` | ✅ | ✅ | — |
| `try/catch/finally` | ✅ | ✅ | COS 风格 setjmp/longjmp |
| `throw` | ✅ | ✅ | — |
| `yield` | ✅ | ❌ | — |
| `instanceof` | ✅ | ✅ | 遍历类链 |
| `fn =>` | ✅ | ✅ | — |
| `match` 多条件 | ✅ | ✅ | — |
| `?->` nullsafe | ✅ | ✅ | — |
| 命名参数 | ✅ | ❌ | 影响性能 |
| `...$args` 可变参数 | ✅ | ❌ | 影响性能 |
| nullable `?type` | ✅ | ⬜ | — |
| union `A\|B` | ✅ | ⬜ | — |
| `__construct` 属性提升 | ✅ | ✅ | — |
| 默认参数值 | ✅ | ⬌ | 部分支持 |
| `declare(strict_types)` | ✅ | ⬜ | 当前跳过 |
| attributes `#[...]` | ✅ | ❌ | — |
| `include`/`require` | ✅ | ❌ | AOT 不可行 |
| `eval()` | ✅ | ❌ | AOT 不可行 |
| `$$var` | ✅ | ❌ | AOT 不可行 |
| `__call/__get/__set` | ✅ | ❌ | AOT 不可行 |
