<?php

class Demo
{
    const string AAA = "hello";
    public const string BBB = "world";
    private const array CCC = [1, 2];

    public function hello(): void
    {
        echo "Hello, Class Demo!\n";
    }
}

class Main
{
    public function main(): void
    {
        echo "===== class const test =====\n";

        $d = new Demo();
        $d->hello();

        echo "\n=== done ===\n";
    }
}
