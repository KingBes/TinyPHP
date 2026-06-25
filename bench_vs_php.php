<?php

class Main
{
    public function main(): void
    {
        $N = 500000;

        echo "============================================================\n";
        echo " TinyPHP vs Native PHP - Performance Benchmark\n";
        echo " Iterations: " . $N . " per test\n";
        echo "============================================================\n\n";

        // ≈≈≈ 1. Loop Overhead ≈≈≈
        echo "--- 1. Loop Overhead ---\n";
        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $x = 1; $x = $x + 1; }
        $t1 = hrtime() - $t0;
        echo 'empty-loop: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        // ≈≈≈ 2. Integer Arithmetic ≈≈≈
        echo "\n--- 2. Integer Arithmetic ---\n";
        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $r = 100 + 200; }
        $t1 = hrtime() - $t0;
        echo 'int-add: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $r = 100 * 200; }
        $t1 = hrtime() - $t0;
        echo 'int-mul: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rf = 1000.0 / 7.0; }
        $t1 = hrtime() - $t0;
        echo 'float-div: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $r = 1000 % 7; }
        $t1 = hrtime() - $t0;
        echo 'int-mod: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        // ≈≈≈ 3. Float Arithmetic ≈≈≈
        echo "\n--- 3. Float Arithmetic ---\n";
        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rf = 1.5 + 2.5; }
        $t1 = hrtime() - $t0;
        echo 'float-add: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rf = 3.14 * 2.718; }
        $t1 = hrtime() - $t0;
        echo 'float-mul: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        // ≈≈≈ 4. String Ops ≈≈≈
        echo "\n--- 4. String Ops ---\n";
        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rs = 'hello ' . 'world'; }
        $t1 = hrtime() - $t0;
        echo 'concat-2: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rs = 'a' . 'b' . 'c' . 'd'; }
        $t1 = hrtime() - $t0;
        echo 'concat-4: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        // ≈≈≈ 5. String Functions ≈≈≈
        echo "\n--- 5. String Functions ---\n";
        $sA = 'hello_world_no_spaces_longer_string_for_testing';
        $sB = '  spaces  ';
        $sC = 'HELLO';

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $r = strlen($sA); }
        $t1 = hrtime() - $t0;
        echo 'strlen: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rs = trim($sA); }
        $t1 = hrtime() - $t0;
        echo 'trim-no-ws: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rs = trim($sB); }
        $t1 = hrtime() - $t0;
        echo 'trim-ws: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rs = strtolower($sA); }
        $t1 = hrtime() - $t0;
        echo 'strtolower-nc: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rs = strtolower($sC); }
        $t1 = hrtime() - $t0;
        echo 'strtolower-c: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rs = strtoupper($sC); }
        $t1 = hrtime() - $t0;
        echo 'strtoupper: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rs = substr($sA, 0, strlen($sA)); }
        $t1 = hrtime() - $t0;
        echo 'substr-full: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rs = substr($sA, 6, 5); }
        $t1 = hrtime() - $t0;
        echo 'substr-part: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $r = strpos($sA, 'world'); }
        $t1 = hrtime() - $t0;
        echo 'strpos: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        // ≈≈≈ 6. Array ≈≈≈
        echo "\n--- 6. Array ---\n";

        $t0 = hrtime();
        for ($j = 0; $j < 50000; $j++) { $a = []; }
        $t1 = hrtime() - $t0;
        echo 'arr-create: ' . $t1 . " ns (" . ($t1 / 50000) . " ns/op)\n";

        $t0 = hrtime();
        for ($j = 0; $j < 50000; $j++) {
            $a = [];
            for ($k = 0; $k < 10; $k++) { array_push($a, $k); }
        }
        $t1 = hrtime() - $t0;
        echo 'arr-push-10: ' . $t1 . " ns (" . ($t1 / 50000) . " ns/op)\n";

        $t0 = hrtime();
        for ($j = 0; $j < 5000; $j++) {
            $a = [];
            for ($k = 0; $k < 100; $k++) { array_push($a, $k); }
        }
        $t1 = hrtime() - $t0;
        echo 'arr-push-100: ' . $t1 . " ns (" . ($t1 / 5000) . " ns/op)\n";

        // ≈≈≈ 7. Array Operations ≈≈≈
        echo "\n--- 7. Array Operations ---\n";
        $arr = [];
        for ($m = 0; $m < 50; $m++) { array_push($arr, $m); }

        $t0 = hrtime();
        for ($j = 0; $j < 5000; $j++) { $a = $arr; sort($a); }
        $t1 = hrtime() - $t0;
        echo 'sort(50): ' . $t1 . " ns (" . ($t1 / 5000) . " ns/op)\n";

        $t0 = hrtime();
        for ($j = 0; $j < 100000; $j++) { $r = array_search(25, $arr); }
        $t1 = hrtime() - $t0;
        echo 'array_search: ' . $t1 . " ns (" . ($t1 / 100000) . " ns/op)\n";

        $t0 = hrtime();
        for ($j = 0; $j < 100000; $j++) { $b = in_array(25, $arr); }
        $t1 = hrtime() - $t0;
        echo 'in_array: ' . $t1 . " ns (" . ($t1 / 100000) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $r = count($arr); }
        $t1 = hrtime() - $t0;
        echo 'count: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $small = [1, 2, 2, 3, 3, 3, 4, 5];
        $t0 = hrtime();
        for ($j = 0; $j < 50000; $j++) { $a = array_unique($small); }
        $t1 = hrtime() - $t0;
        echo 'uniq-small(8): ' . $t1 . " ns (" . ($t1 / 50000) . " ns/op)\n";

        $big = [];
        for ($m = 0; $m < 500; $m++) { array_push($big, $m % 50); }
        $t0 = hrtime();
        for ($j = 0; $j < 5000; $j++) { $a = array_unique($big); }
        $t1 = hrtime() - $t0;
        echo 'uniq-large(500): ' . $t1 . " ns (" . ($t1 / 5000) . " ns/op)\n";

        // ≈≈≈ 8. Math ≈≈≈
        echo "\n--- 8. Math ---\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $r = abs(-12345); }
        $t1 = hrtime() - $t0;
        echo 'abs: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rf = sqrt(2.0); }
        $t1 = hrtime() - $t0;
        echo 'sqrt: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rf = round(3.14159); }
        $t1 = hrtime() - $t0;
        echo 'round: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rf = ceil(3.14); }
        $t1 = hrtime() - $t0;
        echo 'ceil: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        $t0 = hrtime();
        for ($i = 0; $i < $N; $i++) { $rf = floor(3.14); }
        $t1 = hrtime() - $t0;
        echo 'floor: ' . $t1 . " ns (" . ($t1 / $N) . " ns/op)\n";

        // ≈≈≈ 9. JSON ≈≈≈
        echo "\n--- 9. JSON ---\n";
        $data = ['name' => 'test', 'val' => 42, 'flag' => true, 'items' => [1, 2, 3]];
        $jsonStr = json_encode($data);

        $t0 = hrtime();
        for ($j = 0; $j < 10000; $j++) { $rs = json_encode($data); }
        $t1 = hrtime() - $t0;
        echo 'json_encode: ' . $t1 . " ns (" . ($t1 / 10000) . " ns/op)\n";

        $t0 = hrtime();
        for ($j = 0; $j < 50000; $j++) { $a = explode(',', $jsonStr); }
        $t1 = hrtime() - $t0;
        echo 'explode: ' . $t1 . " ns (" . ($t1 / 50000) . " ns/op)\n";

        // ≈≈≈ 10. File I/O ≈≈≈
        echo "\n--- 10. File I/O ---\n";
        $t0 = hrtime();
        for ($j = 0; $j < 1000; $j++) {
            file_put_contents('_bench_tmp.txt', 'hello benchmark');
            $rs = file_get_contents('_bench_tmp.txt');
        }
        $t1 = hrtime() - $t0;
        echo 'file-io: ' . $t1 . " ns (" . ($t1 / 1000) . " ns/op)\n";

        echo "\n=== Benchmark Complete ===\n";
    }
}
