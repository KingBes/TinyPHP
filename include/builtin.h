#pragma once

// builtin.h — TinyPHP 内置函数总入口
// TCC 对多个 #include 的符号表限制是 65535, 230+ inline 函数必超
// 所以全合并到 builtin_full.h 一个文件里 (69KB single unit)
// GCC/Clang 不受影响, std/ 下分文件保留供人类阅读

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "types.h"

#include "builtin_full.h"
