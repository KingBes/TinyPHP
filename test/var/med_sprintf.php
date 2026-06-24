<?php

// sprintf 完整格式化测试 — 动态缓冲区 + 全格式符

class Main
{
    public function main(): void
    {
        $i = 42;
        $f = 3.14159;
        $s = 'world';

        echo "1. simple: "   . sprintf('hello %s', $s)   . "\n";
        echo "2. int: "      . sprintf('answer=%d', $i)  . "\n";
        echo "3. hex: "      . sprintf('0x%x', 255)      . "\n";
        echo "4. octal: "    . sprintf('0%o', 64)        . "\n";
        echo "5. float: "    . sprintf('pi=%.2f', $f)    . "\n";
        echo "6. padded: "   . sprintf('[%5d]', 7)       . "\n";
        echo "7. zeroPad: "  . sprintf('%04d', 42)       . "\n";
        echo "8. multi: "    . sprintf('%s=%d %.2f', $s, $i, $f) . "\n";
        echo "9. percent: "  . sprintf('100%% done')     . "\n";
        echo "10. char: "    . sprintf('A=%c (65)', 65)  . "\n";
        echo "11. long: "    . sprintf('%-20s end', 'left-aligned') . "\n";
        echo "12. sci: "     . sprintf('%e', 1234.5)     . "\n";
    }
}
