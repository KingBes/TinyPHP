#!/usr/bin/env php
<?php
// CI test runner — 收集全部测试结果
// 用法: php run_tests.php [cc_flag]

$ccFlag = $argv[1] ?? '';
$testDir = getenv('GITHUB_WORKSPACE') ?: dirname(__DIR__, 2);
$phpExe  = PHP_OS_FAMILY === 'Windows' ? 'php.exe' : './php';
$tphp    = $testDir . DIRECTORY_SEPARATOR . 'tphp.php';

// 收集所有含 #debug 的测试文件（排除 @skip）
$testFiles = [];
$iter = new RecursiveIteratorIterator(
    new RecursiveDirectoryIterator($testDir . DIRECTORY_SEPARATOR . 'test', FilesystemIterator::SKIP_DOTS)
);
foreach ($iter as $file) {
    if ($file->getExtension() !== 'php') continue;
    $content = file_get_contents($file->getPathname());
    if ($content === false) continue;
    if (str_contains($content, '@skip')) continue;
    if (!str_contains($content, '#debug')) continue;
    $testFiles[] = $file->getPathname();
}
sort($testFiles);

// 运行测试
$platform = PHP_OS_FAMILY . ' ' . php_uname('m');
$compiler = $ccFlag ? ltrim(str_replace('-cc ', '', $ccFlag)) : 'tcc';
echo "Platform: $platform | Compiler: $compiler\n";
echo "Tests: " . count($testFiles) . "\n\n";

$passed   = [];
$failed   = [];
$logDir   = $testDir . DIRECTORY_SEPARATOR . 'build';
if (!is_dir($logDir)) mkdir($logDir, 0777, true);

foreach ($testFiles as $i => $f) {
    $rel    = str_replace('\\', '/', substr($f, strlen($testDir) + 1));
    $base   = basename($f, '.php');
    $out    = $logDir . DIRECTORY_SEPARATOR . 'test_' . $base . (PHP_OS_FAMILY === 'Windows' ? '.exe' : '');
    $log    = $logDir . DIRECTORY_SEPARATOR . 'test_' . $base . '.log';

    $cmd = escapeshellarg($phpExe) . ' ' . escapeshellarg($tphp) . ' '
         . escapeshellarg($f) . ' --debug ' . $ccFlag
         . ' -o ' . escapeshellarg($out)
         . ' >' . escapeshellarg($log) . ' 2>&1';
    system($cmd, $ret);

    if ($ret === 0) {
        $passed[] = $rel;
        @unlink($log);
    } else {
        $errLines = [];
        $logContent = @file_get_contents($log);
        if ($logContent) {
            foreach (explode("\n", $logContent) as $line) {
                if (str_contains($line, '[FAIL]') || str_contains($line, 'expected:') || str_contains($line, 'got     :')) {
                    $errLines[] = rtrim($line);
                }
            }
            // 也捕获编译错误
            if (empty($errLines)) {
                foreach (explode("\n", $logContent) as $line) {
                    if (str_contains($line, 'error:') || str_contains($line, 'Error:')) {
                        $errLines[] = rtrim($line);
                    }
                }
            }
        }
        $failed[$rel] = $errLines;
        @unlink($log);
    }
}

// 输出结果
echo str_repeat('=', 60) . "\n";
echo "PASS: " . count($passed) . " | FAIL: " . count($failed) . "\n";
echo str_repeat('=', 60) . "\n";

if (count($failed) > 0) {
    echo "\n";
    foreach ($failed as $file => $errors) {
        echo "  FAIL: $file\n";
        foreach ($errors as $e) {
            echo "    $e\n";
        }
        echo "\n";
    }
    exit(1);
}

echo "\nAll tests passed.\n";
