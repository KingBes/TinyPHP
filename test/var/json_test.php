<?php

class Main
{
    public function main(): void
    {
        // ============================================================
        // 1. json_encode — 基本类型
        // ============================================================
        echo "===== 1. encode primitives =====\n";

        var_dump(json_encode(null));      // string(4) "null"
        var_dump(json_encode(true));      // string(4) "true"
        var_dump(json_encode(false));     // string(5) "false"
        var_dump(json_encode(42));        // string(2) "42"
        var_dump(json_encode(-7));        // string(2) "-7"
        var_dump(json_encode(3.14));      // string(4) "3.14"
        var_dump(json_encode("hello"));   // string(7) ""hello""

        // ============================================================
        // 2. json_encode — 数组
        // ============================================================
        echo "\n===== 2. encode array =====\n";

        $a = [1, 2, 3];
        var_dump(json_encode($a));        // "[1,2,3]"

        $b = [10, "text", true, null];
        var_dump(json_encode($b));        // "[10,"text",true,null]"

        $empty = [];
        var_dump(json_encode($empty));    // "[]"

        // ============================================================
        // 3. json_encode — 嵌套数组
        // ============================================================
        echo "\n===== 3. encode nested =====\n";

        $nested = [[1, 2], [3, 4]];
        var_dump(json_encode($nested));   // "[[1,2],[3,4]]"

        // ============================================================
        // 4. json_decode — 基本类型
        // ============================================================
        echo "\n===== 4. decode primitives =====\n";

        $d1 = json_decode("42");
        var_dump($d1);                    // int(42)

        $d2 = json_decode("3.14");
        var_dump($d2);                    // float(3.14)

        $d3 = json_decode("true");
        var_dump($d3);                    // bool(true)

        $d4 = json_decode("false");
        var_dump($d4);                    // bool(false)

        $d5 = json_decode("null");
        var_dump($d5);                    // NULL

        $d6 = json_decode('"hello"');
        var_dump($d6);                    // string(5) "hello"

        // ============================================================
        // 5. json_decode — 数组
        // ============================================================
        echo "\n===== 5. decode array =====\n";

        $arr1 = json_decode("[1,2,3]");
        var_dump($arr1);                  // array(3)
        var_dump(is_array($arr1));        // bool(true)

        $arr2 = json_decode('[10, true, null, "x"]');
        var_dump($arr2);                  // array(4)

        // ============================================================
        // 6. json_decode — 对象
        // ============================================================
        echo "\n===== 6. decode object =====\n";

        $obj = json_decode('{"name":"alice","age":30}');
        var_dump($obj);                   // array(2)
        var_dump(is_array($obj));         // bool(true)

        // ============================================================
        // 7. round-trip: encode → decode → encode
        // ============================================================
        echo "\n===== 7. round-trip =====\n";

        $orig = [1, 2, 3, 4, 5];
        $enc  = json_encode($orig);
        $dec  = json_decode($enc);
        $enc2 = json_encode($dec);
        var_dump($enc);                   // "[1,2,3,4,5]"
        var_dump($enc2);                  // "[1,2,3,4,5]"

        // ============================================================
        // 8. 字符串转义
        // ============================================================
        echo "\n===== 8. string escape =====\n";

        echo json_encode('say "hi"') . "\n";    // "say \"hi\""
        echo json_encode("line1\nline2") . "\n";   // "line1\nline2"

        echo "\n=== all json tests done ===\n";
    }
}
