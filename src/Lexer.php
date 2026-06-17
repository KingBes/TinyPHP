<?php

declare(strict_types=1);

class Lexer
{
    private string $source;
    private int $pos = 0;
    private int $line = 1;
    private int $column = 1;

    /** @var Token[] */
    private array $tokens = [];

    private static array $keywords = [
        'class'       => TokenType::CLASS_KW,
        'public'      => TokenType::PUBLIC_KW,
        'private'     => TokenType::PRIVATE_KW,
        'function'    => TokenType::FUNCTION,
        'return'      => TokenType::RETURN_KW,
        'echo'        => TokenType::ECHO_KW,
        'new'         => TokenType::NEW_KW,
        'null'        => TokenType::NULL_KW,
        'true'        => TokenType::TRUE_KW,
        'false'       => TokenType::FALSE_KW,
        'int'         => TokenType::TYPE_INT,
        'float'       => TokenType::TYPE_FLOAT,
        'string'      => TokenType::TYPE_STRING,
        'bool'        => TokenType::TYPE_BOOL,
        'void'        => TokenType::TYPE_VOID,
        'array'       => TokenType::TYPE_ARRAY,
        '__construct' => TokenType::CONSTRUCT,
        '__destruct'  => TokenType::DESTRUCT,
        'var_dump'    => TokenType::VAR_DUMP,
        'count'       => TokenType::COUNT,
        'namespace'   => TokenType::NAMESPACE,
        'use'         => TokenType::USE,
        'as'          => TokenType::AS_KW,
    ];

    public function __construct(string $source)
    {
        $this->source = $source;
    }

    /** @return Token[] */
    public function tokenize(): array
    {
        $this->pos = 0;
        $this->line = 1;
        $this->column = 1;
        $this->tokens = [];

        // 必须在最开头
        if (str_starts_with($this->source, '<?php')) {
            $this->addToken(TokenType::PHP_OPEN, '<?php');
            $this->pos = 5; // skip <?php
            $this->column = 6;
        } else {
            $this->error('源文件必须以 <?php 开头');
        }

        while ($this->pos < strlen($this->source)) {
            $this->scanToken();
        }

        $this->addToken(TokenType::EOF, '');
        return $this->tokens;
    }

    private function scanToken(): void
    {
        $ch = $this->peek();

        // 空白 / 换行（CRLF → 一次 line++，\r 单独视为换行）
        if ($ch === ' ' || $ch === "\t") {
            $this->advance();
            return;
        }
        if ($ch === "\r") {
            if ($this->peek(1) === "\n") $this->advance(); // skip \n of CRLF
            $this->line++;
            $this->column = 1;
            $this->pos++;
            return;
        }
        if ($ch === "\n") {
            $this->line++;
            $this->column = 1;
            $this->pos++;
            return;
        }

        // 注释 / 除号
        if ($ch === '/') {
            if ($this->peek(1) === '/') {
                $this->skipLineComment();
                return;
            }
            if ($this->peek(1) === '*') {
                $this->skipBlockComment();
                return;
            }
            $this->addToken(TokenType::SLASH, '/');
            $this->advance();
            return;
        }

        // -> (必须在单字符运算符之前检查)
        if ($ch === '-' && $this->peek(1) === '>') {
            $this->addToken(TokenType::ARROW, '->');
            $this->advance(2);
            return;
        }

        // 运算符
        $opChars = [
            '+' => TokenType::PLUS,
            '-' => TokenType::MINUS,
            '*' => TokenType::STAR,
        ];
        if (isset($opChars[$ch])) {
            $this->addToken($opChars[$ch], $ch);
            $this->advance();
            return;
        }

        // 字符串
        if ($ch === '"' || $ch === "'") {
            $this->scanString();
            return;
        }

        // 符号
        $singleChars = [
            '(' => TokenType::LPAREN,
            ')' => TokenType::RPAREN,
            '{' => TokenType::LBRACE,
            '}' => TokenType::RBRACE,
            '[' => TokenType::LBRACKET,
            ']' => TokenType::RBRACKET,
            ':' => TokenType::COLON,
            ';' => TokenType::SEMICOLON,
            ',' => TokenType::COMMA,
            '=' => TokenType::EQUALS,
            '.' => TokenType::DOT,
        ];
        // \ 命名空间分隔符
        if ($ch === '\\') {
            $this->addToken(TokenType::NS_SEP, '\\');
            $this->advance();
            return;
        }
        if (isset($singleChars[$ch])) {
            $this->addToken($singleChars[$ch], $ch);
            $this->advance();
            return;
        }

        // $ 变量
        if ($ch === '$') {
            $this->scanVariable();
            return;
        }

        // ::
        if ($ch === ':' && $this->peek(1) === ':') {
            $this->addToken(TokenType::DOUBLE_COLON, '::');
            $this->advance(2);
            return;
        }

        // 数字
        if (ctype_digit($ch)) {
            $this->scanNumber();
            return;
        }

        // 标识符/关键字
        if (ctype_alpha($ch) || $ch === '_') {
            $this->scanIdentifier();
            return;
        }

        $this->error("意外的字符: '{$ch}'");
    }

    private function peek(int $offset = 0): string
    {
        $idx = $this->pos + $offset;
        return ($idx < strlen($this->source)) ? $this->source[$idx] : "\0";
    }

    private function advance(int $n = 1): void
    {
        for ($i = 0; $i < $n; $i++) {
            if ($this->pos < strlen($this->source) && $this->source[$this->pos] === "\n") {
                $this->line++;
                $this->column = 1;
            } else {
                $this->column++;
            }
            $this->pos++;
        }
    }

    private function addToken(TokenType $type, string $lexeme, mixed $literal = null): void
    {
        $this->tokens[] = new Token($type, $lexeme, $this->line, $this->column, $literal);
    }

    private function skipLineComment(): void
    {
        while ($this->pos < strlen($this->source) && $this->peek() !== "\n") {
            $this->advance();
        }
    }

    private function skipBlockComment(): void
    {
        $this->advance(2); // skip /*
        while ($this->pos < strlen($this->source)) {
            if ($this->peek() === '*' && $this->peek(1) === '/') {
                $this->advance(2);
                return;
            }
            $this->advance();
        }
        $this->error('未闭合的块注释');
    }

    private function scanString(): void
    {
        $quote = $this->peek();
        $this->advance(); // skip opening quote

        // 单引号：简单扫描，无插值
        if ($quote === "'") {
            $escaped = '';
            while ($this->pos < strlen($this->source) && $this->peek() !== $quote) {
                $ch = $this->peek();
                if ($ch === '\\' && $this->peek(1) !== "\0") {
                    $this->advance();
                    $next = $this->peek();
                    $escaped .= '\\' . $next;
                    $this->advance();
                } else {
                    $escaped .= $ch;
                    $this->advance();
                }
            }
            if ($this->peek() !== $quote) $this->error('未闭合的字符串');
            $this->advance();
            $this->addToken(TokenType::STRING_LIT, $escaped, $escaped);
            return;
        }

        // 双引号：支持 $var / {$var} 插值，各段之间自动插入 DOT
        $buf = '';
        $needDot = false; // 是否需要在下一段前插入 DOT

        while ($this->pos < strlen($this->source) && $this->peek() !== $quote) {
            $ch = $this->peek();

            if ($ch === '\\' && $this->peek(1) !== "\0") {
                $this->advance();
                $next = $this->peek();
                $buf .= '\\' . $next;
                $this->advance();
                continue;
            }

            // 插值：$var 或 {$var}
            if ($ch === '$') {
                $this->advance(); // skip $
                // {$var} 语法：去掉 buf 末尾的 {
                if ($buf !== '' && $buf[strlen($buf) - 1] === '{') {
                    $buf = substr($buf, 0, -1);
                }
                // 先输出累积的文本段
                if ($buf !== '') {
                    if ($needDot) $this->addToken(TokenType::DOT, '.');
                    $this->addToken(TokenType::STRING_LIT, $buf, $buf);
                    $buf = '';
                    $needDot = true;
                }

                if ($needDot) $this->addToken(TokenType::DOT, '.');

                // {$var} 语法
                if ($this->peek() === '{') $this->advance();

                $varName = '';
                while ($this->pos < strlen($this->source) && (ctype_alnum($this->peek()) || $this->peek() === '_')) {
                    $varName .= $this->peek();
                    $this->advance();
                }

                if ($this->peek() === '}') $this->advance(); // skip }

                $this->addToken(TokenType::IDENTIFIER, '$' . $varName);
                $needDot = true;
                continue;
            }

            $buf .= $ch;
            $this->advance();
        }

        if ($this->peek() !== $quote) $this->error('未闭合的字符串');
        $this->advance(); // skip closing quote

        if ($buf !== '') {
            if ($needDot) $this->addToken(TokenType::DOT, '.');
            $this->addToken(TokenType::STRING_LIT, $buf, $buf);
        } elseif (!$needDot) {
            // 空字符串 "" — 至少输出一个空 STRING_LIT
            $this->addToken(TokenType::STRING_LIT, '', '');
        }
    }

    private function scanNumber(): void
    {
        $num = '';
        $isFloat = false;
        while ($this->pos < strlen($this->source) && (ctype_digit($this->peek()) || $this->peek() === '.')) {
            if ($this->peek() === '.') {
                if ($isFloat) break;
                $isFloat = true;
            }
            $num .= $this->peek();
            $this->advance();
        }

        if ($isFloat) {
            $this->addToken(TokenType::FLOAT_LIT, $num, (float)$num);
        } else {
            $this->addToken(TokenType::INT_LIT, $num, (int)$num);
        }
    }

    private function scanIdentifier(): void
    {
        $name = '';
        while ($this->pos < strlen($this->source) && (ctype_alnum($this->peek()) || $this->peek() === '_')) {
            $name .= $this->peek();
            $this->advance();
        }

        // 检查是否为关键字
        $type = self::$keywords[$name] ?? TokenType::IDENTIFIER;
        $literal = null;
        if ($type === TokenType::TRUE_KW) {
            $literal = true;
        } elseif ($type === TokenType::FALSE_KW) {
            $literal = false;
        } elseif ($type === TokenType::NULL_KW) {
            $literal = null;
        }

        $this->addToken($type, $name, $literal);
    }

    private function scanVariable(): void
    {
        $this->advance(); // skip $
        $name = '';
        while ($this->pos < strlen($this->source) && (ctype_alnum($this->peek()) || $this->peek() === '_')) {
            $name .= $this->peek();
            $this->advance();
        }
        if ($name === 'this') {
            $this->addToken(TokenType::IDENTIFIER, '$this');
        } else {
            $this->addToken(TokenType::IDENTIFIER, '$' . $name);
        }
    }

    private function error(string $msg): never
    {
        throw new RuntimeException(
            sprintf("Lexer 错误 [%d:%d]: %s", $this->line, $this->column, $msg)
        );
    }
}
