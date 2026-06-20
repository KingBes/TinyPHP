<?php

class Main
{
    public function main(): void
    {
        echo "=== test 1: 基础 list ===\n";
        list($a, $b) = [10, 20];
        var_dump($a);   // 10
        var_dump($b);   // 20

        echo "=== test 2: 多余元素忽略 ===\n";
        list($x, $y) = [1, 2, 3, 4];
        var_dump($x);   // 1
        var_dump($y);   // 2

        echo "=== test 3: 跳过元素 ===\n";
        list(, $z) = [100, 200];
        var_dump($z);   // 200

        echo "=== test 4: 连续跳过 ===\n";
        list(, , $w) = [10, 20, 30];
        var_dump($w);   // 30

        echo "=== test 5: 短语法 [] ===\n";
        [$p, $q] = [7, 8];
        var_dump($p);   // 7
        var_dump($q);   // 8

        echo "=== test 6: 短语法跳过 ===\n";
        [, $r] = [99, 100];
        var_dump($r);   // 100

        echo "=== test 7: 嵌套 list ===\n";
        list($u, list($v1, $v2)) = [50, [60, 70]];
        var_dump($u);   // 50
        var_dump($v1);  // 60
        var_dump($v2);  // 70

        echo "=== test 8: 嵌套短语法 ===\n";
        [$m, [$n1, $n2]] = [5, [10, 15]];
        var_dump($m);   // 5
        var_dump($n1);  // 10
        var_dump($n2);  // 15

        echo "=== test 9: 混合嵌套+跳过 ===\n";
        list($h, , list($i1, $i2)) = [1, 2, [3, 4]];
        var_dump($h);   // 1
        var_dump($i1);  // 3
        var_dump($i2);  // 4

        echo "=== test 10: 变量已声明复用 ===\n";
        $prev = 0;
        list($prev) = [42];
        var_dump($prev);  // 42

        echo "=== test 11: 3层嵌套 ===\n";
        [$a1, [$a2, [$a3]]] = [10, [20, [30]]];
        var_dump($a1);  // 10
        var_dump($a2);  // 20
        var_dump($a3);  // 30

        echo "\n=== done ===\n";
    }
}
