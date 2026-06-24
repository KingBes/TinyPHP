<?php

// Parent class
class Animal
{
    public string $name;
    public int $age;

    public function __construct(string $name, int $age)
    {
        $this->name = $name;
        $this->age  = $age;
    }

    public function speak(): string
    {
        return $this->name . ' makes sound';
    }

    public function getAge(): int
    {
        return $this->age;
    }
}

// Child class — extends Animal
class Dog extends Animal
{
    public string $breed;

    public function __construct(string $name, int $age, string $breed)
    {
        $this->name  = $name;
        $this->age   = $age;
        $this->breed = $breed;
    }

    public function speak(): string
    {
        return $this->name . ' barks!';
    }

    public function getBreed(): string
    {
        return $this->breed;
    }
}

class Main
{
    public function main(): void
    {
        echo "=== Extends Test ===\n\n";

        $d = new Dog('Rex', 3, 'Husky');

        // Child method (override)
        echo '1. speak=' . $d->speak() . "\n";

        // Inherited property
        echo '2. name=' . $d->name . "\n";

        // Child property
        echo '3. breed=' . $d->getBreed() . "\n";

        // Inherited method
        echo '4. age=' . $d->getAge() . "\n";

        echo "\n=== OK ===\n";
    }
}
