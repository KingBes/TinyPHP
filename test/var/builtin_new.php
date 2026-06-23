<?php

class Main {
    public function main(): void {
        // ── 1. array_sum / array_product ──
        $a1 = [10, 20, 30];
        echo "1. sum="; var_dump(array_sum($a1));
        echo " prod="; var_dump(array_product($a1));
        echo "\n";

        // ── 2. array_shift ──
        $a2 = [100, 200, 300];
        echo "2. shift="; var_dump(array_shift($a2));
        echo " remaining=" . count($a2) . "\n";

        // ── 3. array_unshift ──
        $a3 = [2, 3];
        $nu = array_unshift($a3, 1);
        echo "3. unshift newlen=$nu a3[0]=" . $a3[0] . "\n";

        // ── 4. array_reverse ──
        $a4 = [1, 2, 3, 4];
        $r4 = array_reverse($a4);
        echo "4. reverse: " . $r4[0] . $r4[1] . $r4[2] . $r4[3] . "\n";

        // ── 5. array_slice ──
        $a5 = [10, 20, 30, 40, 50];
        $s5 = array_slice($a5, 1, 3);
        echo "5. slice[1:3] len=" . count($s5) . " [0]=" . $s5[0] . "\n";

        $s5b = array_slice($a5, -2, 0);
        echo "5b. slice[-2:] len=" . count($s5b) . " [0]=" . $s5b[0] . "\n";

        // ── 6. max / min ──
        $a6 = [5, 99, 3, 77, 1];
        echo "6. max="; var_dump(max($a6));
        echo " min="; var_dump(min($a6));
        echo "\n";

        // ── 7. strlen ──
        echo "7. strlen(hello)=" . strlen("hello") . "\n";

        // ── 8. trim / ltrim / rtrim ──
        $s8 = "  ab c  ";
        echo "8. trim='" . trim($s8) . "' ltrim='" . ltrim($s8) . "' rtrim='" . rtrim($s8) . "'\n";

        // ── 9. substr ──
        $s9 = "hello world";
        echo "9. substr(6,5)='" . substr($s9, 6, 5) . "' substr(-5)='" . substr($s9, -5, 0) . "'\n";

        // ── 10. strpos / str_contains ──
        $s10 = "abcabc";
        echo "10. strpos(ca)=" . strpos($s10, "ca") . " contains(ca)=" . str_contains($s10, "ca") . " contains(xx)=" . str_contains($s10, "xx") . "\n";
    }
}
