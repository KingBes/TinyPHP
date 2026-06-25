<?php

class Main {
    public function main(): void {
        echo "=== Coverage Gap Test ===\n\n";

        // 1. max/min
        echo "-- 1. max/min --\n";
        $a = [5, 99, 3, 42, 7];
        echo "max="; var_dump(max($a));
        echo "min="; var_dump(min($a));

        // 2. sort/rsort with element read
        echo "\n-- 2. sort --\n";
        $s = [30, 10, 20];
        sort($s);
        echo "sorted: " . $s[0] . "," . $s[1] . "," . $s[2] . "\n";
        rsort($s);
        echo "rsorted: " . $s[0] . "," . $s[1] . "," . $s[2] . "\n";

        // 3. range
        echo "\n-- 3. range --\n";
        $r = range(1, 5);
        echo "len=" . count($r) . " r[0]=" . $r[0] . " r[4]=" . $r[4] . "\n";
        $rd = range(10, 1, -3);
        echo "rdesc len=" . count($rd) . " r[0]=" . $rd[0] . "\n";

        // 4. array_fill
        echo "\n-- 4. array_fill --\n";
        $f = array_fill(0, 3, 99);
        echo "fill[0]=" . $f[0] . " fill[2]=" . $f[2] . "\n";

        // 5. array_reverse / array_unique
        echo "\n-- 5. reverse/unique --\n";
        $rv = array_reverse([1,2,3]);
        echo "reverse: " . $rv[0] . $rv[1] . $rv[2] . "\n";
        $uq = array_unique([1,2,2,3,3,3]);
        echo "unique len=" . count($uq) . " uq[0]=" . $uq[0] . "\n";

        // 6. array_slice
        echo "\n-- 6. slice --\n";
        $sl = array_slice([10,20,30,40], 1, 2);
        echo "slice len=" . count($sl) . " s[0]=" . $sl[0] . "\n";

        // 7. str_contains / str_replace
        echo "\n-- 7. str contains/replace --\n";
        echo "contains: " . str_contains("hello", "ll") . " " . str_contains("hello", "xx") . "\n";
        echo "replace: " . str_replace("a", "X", "abcabc") . "\n";

        // 8. strtolower/strtoupper
        echo "\n-- 8. str case --\n";
        echo "lower: " . strtolower("Hello") . "\n";
        echo "upper: " . strtoupper("Hello") . "\n";

        // 9. abs/round/ceil/floor/sqrt
        echo "\n-- 9. math --\n";
        echo "abs: " . abs(-42) . " round: " . round(3.6) . "\n";
        echo "ceil: " . ceil(3.1) . " floor: " . floor(3.9) . "\n";
        echo "sqrt(25)=" . sqrt(25) . "\n";

        // 10. array_search
        echo "\n-- 10. array_search --\n";
        $srch = [100, 200, 300];
        echo "search 200: " . array_search(200, $srch) . "\n";
        echo "search 999: " . array_search(999, $srch) . " (-1=not found)\n";

        echo "\n=== Gap OK ===\n";
    }
}
