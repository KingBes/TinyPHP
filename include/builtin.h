#pragma once

// builtin.h — TinyPHP 内置函数总入口
//   所有函数定义在 include/std/ 下（9个文件合并为 builtins.h）
//   TCC 对多个嵌套 #include 内联有 bug，合并为单文件避免

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "types.h"

#include "std/builtins.h"
