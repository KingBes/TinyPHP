<?php
/**
 * TinyPHP CI Test Runner
 * 
 * 扫描 test/ 目录，找到所有包含 #debug 的测试文件，
 * 逐个编译运行，比对预期与实际输出。
 * 
 * 用法: php ci/test_runner.php
 * 环境变量: TPHP_CC — 指定编译器 (tcc | gcc | clang)，默认 tcc
 */

declare(strict_types=1);

$baseDir = dirname(__DIR__);
$tphpBin = PHP_BINARY . ' ' . escapeshellarg($baseDir . DIRECTORY_SEPARATOR . 'tphp.php');
$cc = getenv('TPHP_CC') ?: 'tcc';

// 收集所有含 #debug 的文件
$files = [];
$it = new RecursiveIteratorIterator(
    new RecursiveDirectoryIterator($baseDir . '/test', RecursiveDirectoryIterator::SKIP_DOTS)
);
foreach ($it as $file) {
    if ($file->getExtension() !== 'php' || !$file->isFile()) {
        continue;
    }
    $content = file_get_contents($file->getRealPath());
    if ($content === false || !preg_match('/^#debug /m', $content)) {
        continue;
    }
    $files[] = $file->getRealPath();
}

if (empty($files)) {
    echo "No test files with #debug found.\n";
    exit(0);
}

echo "╔══════════════════════════════════════╗\n";
echo "║  TinyPHP CI — Compiler: {$cc}        ║\n";
echo "║  Tests: " . str_pad((string)count($files), 27) . "║\n";
echo "╚══════════════════════════════════════╝\n\n";

$passed  = 0;
$failed  = 0;
$skipped = 0;
$total   = count($files);

foreach ($files as $file) {
    $relPath = str_replace($baseDir . DIRECTORY_SEPARATOR, '', $file);
    $name = basename(dirname($file)) . '/' . basename($file);
    echo str_pad("[{$name}]", 50, '.');

    // 构建命令
    $cmd = $tphpBin . ' ' . escapeshellarg($file) . ' --debug';
    if ($cc !== 'tcc') {
        $cmd .= ' -cc ' . escapeshellarg($cc);
    }
    $cmd .= ' -o ' . escapeshellarg(sys_get_temp_dir() . '/tphp_ci_' . getmypid() . '.exe');
    $cmd .= ' 2>&1';

    exec($cmd, $output, $ret);

    // 编译失败
    if ($ret !== 0) {
        echo " BUILD FAIL\n";
        foreach ($output as $line) {
            echo "       {$line}\n";
        }
        $failed++;
        continue;
    }

    // 解析 [YES]/[NO]
    $hasNo = false;
    $failures = [];
    foreach ($output as $line) {
        if (str_starts_with($line, '[NO]')) {
            $hasNo = true;
            $failures[] = $line;
        }
    }

    if ($hasNo) {
        echo " MISMATCH\n";
        foreach ($failures as $f) {
            echo "       {$f}\n";
        }
        $failed++;
    } else {
        echo " PASS\n";
        $passed++;
    }
}

echo "\n═══════════════════════════════════════\n";
echo "  Results: {$passed} passed, {$failed} failed, {$skipped} skipped (of {$total})\n";
echo "═══════════════════════════════════════\n";

exit($failed > 0 ? 1 : 0);
