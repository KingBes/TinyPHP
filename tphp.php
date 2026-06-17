#!/usr/bin/env php
<?php

declare(strict_types=1);

// ============================================================
// TinyPHP — PHP → C 转译编译器
//
// 用法:
//   php tphp.php <file.php> [-o <output.exe>]
//   php tphp.php -f <file.php> [-o <output.exe>]
//
//   将 .php 转为安全的 C 代码，再调用根目录 tcc 编译为 .exe
//   -o 默认值: 与输入文件同名的 .exe（在同目录）
// ============================================================

spl_autoload_register(function (string $class): void {
    $baseDir = __DIR__ . '/src';
    $parts = explode('\\', $class);
    $file = $baseDir . '/' . implode('/', $parts) . '.php';
    if (file_exists($file)) {
        require_once $file;
    }
});

// 手动加载全局命名空间的类
require_once __DIR__ . '/src/TokenType.php';
require_once __DIR__ . '/src/Token.php';
require_once __DIR__ . '/src/AST/Node.php';
require_once __DIR__ . '/src/Lexer.php';
require_once __DIR__ . '/src/Parser.php';
require_once __DIR__ . '/src/CodeGenerator.php';
require_once __DIR__ . '/src/Compiler.php';

// --- 参数解析 ---
$options = getopt('f:o:h', ['help']);
$phpFile = $options['f'] ?? $argv[1] ?? '';
$outExe  = $options['o'] ?? '';

if (isset($options['h']) || isset($options['help']) || $phpFile === '') {
    showHelp();
}

// 解析 -o
if ($outExe === '') {
    $outExe = dirname($phpFile) . DIRECTORY_SEPARATOR . pathinfo($phpFile, PATHINFO_FILENAME) . '.exe';
}
$outDir  = dirname($outExe);
$outBase = pathinfo($outExe, PATHINFO_FILENAME);

// 路径
$includeDir = __DIR__ . DIRECTORY_SEPARATOR . 'include';
$tccExe     = __DIR__ . DIRECTORY_SEPARATOR . 'tcc' . DIRECTORY_SEPARATOR . 'tcc.exe';

// 检查文件
if (!file_exists($phpFile)) {
    die("错误: PHP 文件不存在: {$phpFile}\n");
}
if (!is_dir($includeDir)) {
    die("错误: include 目录不存在: {$includeDir}\n");
}
if (!file_exists($tccExe)) {
    die("错误: TCC 编译器未找到: {$tccExe}\n");
}

// --- Phase 1: 转译 PHP → C ---
echo "[1/3] 转译 {$phpFile} → C 代码...\n";

try {
    $source = file_get_contents($phpFile);
    if ($source === false || trim($source) === '') {
        die("错误: PHP 文件为空\n");
    }
    if (!is_dir($outDir)) {
        mkdir($outDir, 0777, true);
    }

    $lexer  = new Lexer($source);
    $tokens = $lexer->tokenize();

    $parser = new Parser($tokens);
    $ast    = $parser->parse();

    $gen   = new CodeGenerator();
    $cFile = $gen->generate($ast, $phpFile, $outDir);

    echo "       ✓ {$cFile}\n";

} catch (RuntimeException $e) {
    die("✗ 转译失败: " . $e->getMessage() . "\n");
}

// --- Phase 2: TCC 编译 C → .exe ---
echo "[2/3] TCC 编译 → {$outExe}...\n";

$cmd = sprintf(
    '"%s" -I"%s" -o "%s" "%s" 2>&1',
    $tccExe,
    $includeDir,
    $outExe,
    $cFile
);

$tccOutput = [];
$retval = 0;
exec($cmd, $tccOutput, $retval);

if ($retval !== 0) {
    echo "✗ TCC 编译失败:\n";
    echo implode("\n", $tccOutput) . "\n";
    exit(1);
}

echo "       ✓ {$outExe}\n";

// --- Phase 3: 运行 ---
echo "[3/3] 运行...\n";
echo "────────────────────────────────────────\n";

passthru('"' . $outExe . '"', $runRet);

echo "\n────────────────────────────────────────\n";
echo "退出码: {$runRet}\n";

// ============================================================
function showHelp(): never
{
    echo <<<HELP
TinyPHP — PHP → C 转译编译器

用法:
  php tphp.php <file.php> [-o <output.exe>]

选项:
  -o <output.exe>  输出的 .exe 文件路径 (可选，默认同目录同名 .exe)
  -h, --help       显示帮助

示例:
  php tphp.php test/main/main.php
  php tphp.php test/main/main.php -o build/main.exe

HELP;
    exit(0);
}
