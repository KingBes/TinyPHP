<?php

declare(strict_types=1);

/**
 * 递归下降解析器
 *
 * 语法（简化）:
 *   program     → PHP_OPEN class_decl EOF
 *   class_decl  → CLASS_KW IDENTIFIER LBRACE method* RBRACE
 *   method      → visibility FUNCTION (IDENTIFIER | CONSTRUCT | DESTRUCT)
 *                    LPAREN params? RPAREN COLON type LBRACE stmt* RBRACE
 *   params      → param (COMMA param)*
 *   param       → type DOLLAR IDENTIFIER
 *   stmt        → echo_stmt | return_stmt | assign_stmt | expr_stmt
 *   echo_stmt   → ECHO_KW expr (COMMA expr)* SEMICOLON
 *   return_stmt → RETURN_KW expr? SEMICOLON
 *   assign_stmt → IDENTIFIER EQUALS expr SEMICOLON
 *   expr_stmt   → expr SEMICOLON
 *   expr        → primary (ARROW call | LPAREN args RPAREN)?
 *   primary     → STRING_LIT | INT_LIT | FLOAT_LIT | TRUE_KW | FALSE_KW
 *                | NULL_KW | IDENTIFIER | NEW_KW IDENTIFIER LPAREN args? RPAREN
 *                | LPAREN type RPAREN expr
 *   args        → expr (COMMA expr)*
 *   type        → TYPE_INT | TYPE_FLOAT | TYPE_STRING | TYPE_BOOL
 *                | TYPE_VOID | TYPE_ARRAY | IDENTIFIER
 *   visibility  → PUBLIC_KW | PRIVATE_KW
 */
class Parser
{
    /** @var Token[] */
    private array $tokens;
    private int $current = 0;

    /** @param Token[] $tokens */
    public function __construct(array $tokens)
    {
        $this->tokens = $tokens;
    }

    public function parse(): ProgramNode
    {
        $this->current = 0;
        $program = $this->parseProgram();

        if (!$this->check(TokenType::EOF)) {
            $this->error('期望文件结束，得到 ' . $this->peek()->lexeme);
        }

        return $program;
    }

    // ============================================================
    // program → PHP_OPEN class_decl (class_decl | function)* EOF
    // ============================================================
    private function parseProgram(): ProgramNode
    {
        $this->consume(TokenType::PHP_OPEN, '期望 <?php 开头');

        // 第一个类 = Main（入口）
        $mainClass = $this->parseClassDecl();

        // 后续：辅助类 / 独立函数（可交错）
        $extraClasses = [];
        $functions = [];
        while (!$this->check(TokenType::EOF)) {
            if ($this->check(TokenType::CLASS_KW)) {
                $extraClasses[] = $this->parseClassDecl();
            } elseif ($this->check(TokenType::FUNCTION)) {
                $functions[] = $this->parseFunction();
            } else {
                $this->error('期望 class 或 function，得到 ' . $this->peek()->lexeme);
            }
        }

        return new ProgramNode($mainClass, $extraClasses, $functions);
    }

    // ============================================================
    // function_decl → FUNCTION IDENTIFIER LPAREN params? RPAREN COLON? type? LBRACE stmt* RBRACE
    // ============================================================
    private function parseFunction(): FunctionNode
    {
        $this->consume(TokenType::FUNCTION, '期望 function 关键字');

        $name = $this->consume(TokenType::IDENTIFIER, '期望函数名')->lexeme;

        $this->consume(TokenType::LPAREN, '期望 (');
        $params = [];
        if (!$this->check(TokenType::RPAREN)) {
            $params = $this->parseParams();
        }
        $this->consume(TokenType::RPAREN, '期望 )');

        $returnType = 'void';
        if ($this->match(TokenType::COLON)) {
            $returnType = $this->parseType();
        }

        $this->consume(TokenType::LBRACE, '期望 {');
        $body = [];
        while (!$this->check(TokenType::RBRACE) && !$this->check(TokenType::EOF)) {
            $body[] = $this->parseStmt();
        }
        $this->consume(TokenType::RBRACE, '期望 }');

        return new FunctionNode($name, $params, $returnType, $body);
    }

    // ============================================================
    // class_decl → CLASS_KW IDENTIFIER LBRACE method* RBRACE
    // ============================================================
    private function parseClassDecl(): ClassNode
    {
        $this->consume(TokenType::CLASS_KW, '期望 class 关键字');
        $name = $this->consume(TokenType::IDENTIFIER, '期望类名')->lexeme;
        $this->consume(TokenType::LBRACE, '期望 {');

        $methods = [];
        while (!$this->check(TokenType::RBRACE) && !$this->check(TokenType::EOF)) {
            $methods[] = $this->parseMethod();
        }

        $this->consume(TokenType::RBRACE, '期望 }');
        return new ClassNode($name, $methods);
    }

    // ============================================================
    // method → visibility FUNCTION name LPAREN params? RPAREN COLON type LBRACE stmt* RBRACE
    // ============================================================
    private function parseMethod(): MethodNode
    {
        $visibility = $this->parseVisibility();

        $this->consume(TokenType::FUNCTION, '期望 function 关键字');

        // name: IDENTIFIER | CONSTRUCT | DESTRUCT
        $nameToken = $this->advance();
        if (!in_array($nameToken->type, [TokenType::IDENTIFIER, TokenType::CONSTRUCT, TokenType::DESTRUCT], true)) {
            $this->error("期望方法名，得到 '{$nameToken->lexeme}'");
        }
        $name = $nameToken->lexeme;

        // 参数
        $this->consume(TokenType::LPAREN, '期望 (');
        $params = [];
        if (!$this->check(TokenType::RPAREN)) {
            $params = $this->parseParams();
        }
        $this->consume(TokenType::RPAREN, '期望 )');

        // 返回类型
        $returnType = 'void';
        if ($this->match(TokenType::COLON)) {
            $returnType = $this->parseType();
        }

        // 方法体
        $this->consume(TokenType::LBRACE, '期望 {');
        $body = [];
        while (!$this->check(TokenType::RBRACE) && !$this->check(TokenType::EOF)) {
            $body[] = $this->parseStmt();
        }
        $this->consume(TokenType::RBRACE, '期望 }');

        return new MethodNode($name, $visibility, $params, $returnType, $body);
    }

    // ============================================================
    // 参数, 语句, 表达式...
    // ============================================================

    /** @return ParamNode[] */
    private function parseParams(): array
    {
        $params = [];
        do {
            $params[] = $this->parseParam();
        } while ($this->match(TokenType::COMMA));
        return $params;
    }

    private function parseParam(): ParamNode
    {
        $type = $this->parseType();
        $this->consume(TokenType::IDENTIFIER, '期望参数名'); // Lexer 已将 $name 作为 IDENTIFIER
        // 获取上一个 token
        $varName = $this->previous()->lexeme; // e.g. '$argc'
        return new ParamNode($type, $varName);
    }

    private function parseStmt(): StmtNode
    {
        if ($this->match(TokenType::ECHO_KW)) {
            return $this->parseEchoStmt();
        }
        if ($this->match(TokenType::RETURN_KW)) {
            return $this->parseReturnStmt();
        }
        // 赋值: $var = expr;
        if ($this->check(TokenType::IDENTIFIER) && $this->checkNext(TokenType::EQUALS)) {
            return $this->parseAssignStmt();
        }
        // 表达式语句
        return $this->parseExprStmt();
    }

    private function parseEchoStmt(): EchoStmtNode
    {
        $exprs = [];
        do {
            $exprs[] = $this->parseExpr();
        } while ($this->match(TokenType::COMMA));
        $this->consume(TokenType::SEMICOLON, '期望 ;');
        return new EchoStmtNode($exprs);
    }

    private function parseReturnStmt(): ReturnStmtNode
    {
        $expr = null;
        if (!$this->check(TokenType::SEMICOLON)) {
            $expr = $this->parseExpr();
        }
        $this->consume(TokenType::SEMICOLON, '期望 ;');
        return new ReturnStmtNode($expr);
    }

    private function parseAssignStmt(): AssignStmtNode
    {
        $varName = $this->advance()->lexeme;
        $this->consume(TokenType::EQUALS, '期望 =');
        $expr = $this->parseExpr();
        $this->consume(TokenType::SEMICOLON, '期望 ;');
        return new AssignStmtNode($varName, $expr);
    }

    private function parseExprStmt(): ExprStmtNode
    {
        $expr = $this->parseExpr();
        $this->consume(TokenType::SEMICOLON, '期望 ;');
        return new ExprStmtNode($expr);
    }

    private function parseExpr(): ExprNode
    {
        return $this->parseAdditive();
    }

    // additive → multiplicative ((PLUS|MINUS) multiplicative)*
    private function parseAdditive(): ExprNode
    {
        $left = $this->parseMultiplicative();
        while ($this->match(TokenType::PLUS) || $this->match(TokenType::MINUS)) {
            $op = $this->previous()->lexeme;
            $right = $this->parseMultiplicative();
            $left = new BinaryExpr($left, $op, $right);
        }
        return $left;
    }

    // multiplicative → primary ((STAR|SLASH) primary)*
    private function parseMultiplicative(): ExprNode
    {
        $left = $this->parsePrimary();
        while ($this->match(TokenType::STAR) || $this->match(TokenType::SLASH)) {
            $op = $this->previous()->lexeme;
            $right = $this->parsePrimary();
            $left = new BinaryExpr($left, $op, $right);
        }
        return $left;
    }

    private function parsePrimary(): ExprNode
    {
        // 字符串
        if ($this->match(TokenType::STRING_LIT)) {
            return new StringLiteralExpr((string)$this->previous()->literal);
        }
        // 数字
        if ($this->match(TokenType::INT_LIT)) {
            return new IntLiteralExpr((int)$this->previous()->literal);
        }
        if ($this->match(TokenType::FLOAT_LIT)) {
            return new FloatLiteralExpr((float)$this->previous()->literal);
        }
        // 布尔 / null
        if ($this->match(TokenType::TRUE_KW)) {
            return new BoolLiteralExpr(true);
        }
        if ($this->match(TokenType::FALSE_KW)) {
            return new BoolLiteralExpr(false);
        }
        if ($this->match(TokenType::NULL_KW)) {
            return new NullLiteralExpr();
        }
        // new ClassName(args)
        if ($this->match(TokenType::NEW_KW)) {
            return $this->parseNewExpr();
        }
        // 类型转换: (type) expr
        if ($this->check(TokenType::LPAREN)) {
            $nextType = $this->peek(1);
            $typeTokens = [
                TokenType::TYPE_INT, TokenType::TYPE_FLOAT, TokenType::TYPE_STRING,
                TokenType::TYPE_BOOL, TokenType::TYPE_ARRAY,
            ];
            if (in_array($nextType->type, $typeTokens, true)) {
                return $this->parseCastExpr();
            }
        }
        // 变量
        if ($this->check(TokenType::IDENTIFIER)) {
            $name = $this->advance()->lexeme;

            // 方法调用: $obj->method(args) 或 Class::method(args)
            if ($this->match(TokenType::ARROW) || $this->match(TokenType::DOUBLE_COLON)) {
                $methodName = $this->consume(TokenType::IDENTIFIER, '期望方法名')->lexeme;
                $this->consume(TokenType::LPAREN, '期望 (');
                $args = $this->parseArgs();
                $this->consume(TokenType::RPAREN, '期望 )');
                return new CallExpr(new VariableExpr($name), $methodName, $args);
            }

            // 普通函数调用
            if ($this->match(TokenType::LPAREN)) {
                $args = $this->parseArgs();
                $this->consume(TokenType::RPAREN, '期望 )');
                return new CallExpr(null, $name, $args);
            }

            return new VariableExpr($name);
        }

        $this->error("期望表达式，得到 '{$this->peek()->lexeme}'");
    }

    /** @return ExprNode[] */
    private function parseArgs(): array
    {
        $args = [];
        if (!$this->check(TokenType::RPAREN)) {
            do {
                $args[] = $this->parseExpr();
            } while ($this->match(TokenType::COMMA));
        }
        return $args;
    }

    private function parseNewExpr(): NewExpr
    {
        $className = $this->consume(TokenType::IDENTIFIER, '期望类名')->lexeme;
        $this->consume(TokenType::LPAREN, '期望 (');
        $args = $this->parseArgs();
        $this->consume(TokenType::RPAREN, '期望 )');
        return new NewExpr($className, $args);
    }

    private function parseCastExpr(): CastExpr
    {
        $this->consume(TokenType::LPAREN, '期望 (');
        $typeToken = $this->advance();
        $castType = $typeToken->lexeme; // int, float, string, bool, array
        $this->consume(TokenType::RPAREN, '期望 )');
        $expr = $this->parseExpr();
        return new CastExpr($castType, $expr);
    }

    // ============================================================
    // 辅助方法
    // ============================================================

    private function parseVisibility(): string
    {
        if ($this->match(TokenType::PUBLIC_KW)) return 'public';
        if ($this->match(TokenType::PRIVATE_KW)) return 'private';
        $this->error('期望 public 或 private');
    }

    private function parseType(): string
    {
        $typeToken = $this->advance();
        $validTypes = [
            TokenType::TYPE_INT, TokenType::TYPE_FLOAT, TokenType::TYPE_STRING,
            TokenType::TYPE_BOOL, TokenType::TYPE_VOID, TokenType::TYPE_ARRAY,
            TokenType::IDENTIFIER, // 类名作为类型
        ];
        if (!in_array($typeToken->type, $validTypes, true)) {
            $this->error("期望类型声明，得到 '{$typeToken->lexeme}'");
        }
        return $typeToken->lexeme;
    }

    // === Token 操作 ===

    private function peek(int $offset = 0): Token
    {
        return $this->tokens[$this->current + $offset] ?? $this->tokens[count($this->tokens) - 1];
    }

    private function advance(): Token
    {
        return $this->tokens[$this->current++];
    }

    private function previous(): Token
    {
        return $this->tokens[$this->current - 1];
    }

    private function check(TokenType $type): bool
    {
        return $this->peek()->type === $type;
    }

    private function checkNext(TokenType $type): bool
    {
        return $this->peek(1)->type === $type;
    }

    private function match(TokenType $type): bool
    {
        if ($this->check($type)) {
            $this->advance();
            return true;
        }
        return false;
    }

    private function consume(TokenType $type, string $errMsg): Token
    {
        if ($this->check($type)) {
            return $this->advance();
        }
        $this->error($errMsg . "，得到 '{$this->peek()->lexeme}'");
    }

    private function error(string $msg): never
    {
        $tok = $this->peek();
        throw new RuntimeException(
            sprintf("Parser 错误 [%d:%d]: %s", $tok->line, $tok->column, $msg)
        );
    }
}
