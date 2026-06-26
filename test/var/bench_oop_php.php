<?php // @skip — native PHP OOP benchmark

// ============================================================
// PHP Native OOP Benchmark — 与 bench_oop.php 完全对应
// ============================================================

declare(strict_types=1);

class Animal
{
    public string $name;
    public int $age;
    public string $species;

    public function __construct(string $species)
    {
        $this->species = $species;
    }

    public function speak(): string { return $this->name; }
    public function ageInMonths(): int { return $this->age * 12; }
    public function describe(): string { return $this->species; }
}

class Dog extends Animal
{
    public string $breed;
    public function speak(): string { return $this->name; }
    public function bark(): string { return $this->breed; }
}

interface Identifiable { public function id(): int; }

class User implements Identifiable
{
    public int $uid;
    public string $username;
    public function id(): int { return $this->uid; }
}

class Point
{
    public float $x;
    public float $y;
    public function distance(Point $other): float
    {
        $dx = $this->x - $other->x;
        $dy = $this->y - $other->y;
        return sqrt($dx * $dx + $dy * $dy);
    }
}

$N = 500000;

echo "=== PHP Native OOP Benchmark ===\n\n";

// 1. 对象创建+释放
echo "-- 1. new+unset Dog() x{$N} --\n";
$t1 = hrtime(true);
for ($i = 0; $i < $N; $i++) { $d = new Dog('canine'); unset($d); }
$e1 = hrtime(true) - $t1;
echo "create: {$e1} ns\n";

// 2. 属性写入
echo "\n-- 2. property write x{$N} --\n";
$dog = new Dog('canine');
$t1 = hrtime(true);
for ($i = 0; $i < $N; $i++) {
    $dog->name = 'Buddy';
    $dog->age = 3;
    $dog->breed = 'Golden';
}
$e2 = hrtime(true) - $t1;
echo "prop write: {$e2} ns\n";

// 3. 属性读取
echo "\n-- 3. property read x{$N} --\n";
$dog->name = 'Max';
$dog->age = 5;
$t1 = hrtime(true);
$sum = 0;
for ($i = 0; $i < $N; $i++) { $sum += $dog->age; }
$e3 = hrtime(true) - $t1;
echo "prop read: {$e3} ns  (sum={$sum})\n";

// 4. 方法调用 (无参)
echo "\n-- 4. method call (no arg) x{$N} --\n";
$dog->name = 'Rex';
$t1 = hrtime(true);
for ($i = 0; $i < $N; $i++) { $r = $dog->speak(); }
$e4 = hrtime(true) - $t1;
echo "method(0): {$e4} ns\n";

// 5. 方法调用 (有参)
echo "\n-- 5. method call (with arg) x{$N} --\n";
$t1 = hrtime(true);
for ($i = 0; $i < $N; $i++) { $m = $dog->ageInMonths(); }
$e5 = hrtime(true) - $t1;
echo "method(1): {$e5} ns\n";

// 6. 继承方法调用
echo "\n-- 6. inherited method x{$N} --\n";
$t1 = hrtime(true);
for ($i = 0; $i < $N; $i++) { $s = $dog->describe(); }
$e6 = hrtime(true) - $t1;
echo "inherited: {$e6} ns\n";

// 7. 方法链调用
echo "\n-- 7. method chaining x{$N} --\n";
$t1 = hrtime(true);
$totalM = 0;
for ($i = 0; $i < $N; $i++) { $totalM = $dog->ageInMonths(); }
$e7 = hrtime(true) - $t1;
echo "chain: {$e7} ns  (months={$totalM})\n";

// 8. 构造器调用+释放
echo "\n-- 8. constructor+unset x{$N} --\n";
$t1 = hrtime(true);
for ($i = 0; $i < $N; $i++) { $a = new Animal('mammal'); unset($a); }
$e8 = hrtime(true) - $t1;
echo "construct: {$e8} ns\n";

// 9. 接口实现类调用
echo "\n-- 9. interface impl x{$N} --\n";
$user = new User();
$user->uid = 42;
$t1 = hrtime(true);
$uidSum = 0;
for ($i = 0; $i < $N; $i++) { $uidSum = $user->id(); }
$e9 = hrtime(true) - $t1;
echo "impl: {$e9} ns  (uid={$uidSum})\n";

// 10. 对象间方法调用
echo "\n-- 10. inter-object call x" . ($N/10) . " --\n";
$M4 = $N / 10;
$p1 = new Point();
$p1->x = 0.0;
$p1->y = 0.0;
$p2 = new Point();
$p2->x = 3.0;
$p2->y = 4.0;
$t1 = hrtime(true);
$dist = 0.0;
for ($i = 0; $i < $M4; $i++) { $dist = $p1->distance($p2); }
$e10 = hrtime(true) - $t1;
echo "inter-obj: {$e10} ns  (dist={$dist})\n";

echo "\nDone.\n";
