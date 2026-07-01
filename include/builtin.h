#pragma once

// builtin.h — TinyPHP 内置函数总入口
//   全部函数合并在 builtin_full.h, static 已去掉 (TCC 内部符号限制 65535)
//   inline 函数进 .text 段, 不受 TCC 符号表限制

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "types.h"

#include "builtin_full.h"
