<?php

// @skip — pcntl is POSIX-only, tested on Linux/macOS CI

class Main {
    public function main(): void {
        // ── 1. pcntl_fork ──
        echo "-- 1. pcntl_fork --\n";
        $pid = pcntl_fork();
        if ($pid == -1) {
            echo "fork failed\n";
        } elseif ($pid == 0) {
            echo "child pid=" . getmypid() . "\n";
            exit(0);
        } else {
            echo "parent pid=" . $pid . "\n";
            $st = 0;
            pcntl_waitpid($pid, $st, 0);
            echo "child exit\n";
        }

        // ── 2. pcntl_get_last_error / strerror ──
        echo "-- 2. pcntl error --\n";
        echo "errno=" . pcntl_get_last_error() . "\n";
        echo "strerr="; var_dump(pcntl_strerror(0));

        // ── 3. pcntl_alarm ──
        echo "-- 3. pcntl_alarm --\n";
        echo "alarm(0)=" . pcntl_alarm(0) . "\n";

        echo "\n=== pcntl OK ===\n";
    }
}
