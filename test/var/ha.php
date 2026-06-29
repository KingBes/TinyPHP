<?php

class Main {
    public function main(): void {
        $a = <<<'HTML'
<h1>Hello, World!</h1>
HTML;
        echo "Hello, World!\n";
        var_dump($a);
    }
}