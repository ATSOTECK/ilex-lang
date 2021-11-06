package com.nice;

import java.util.List;

import static com.nice.TokenType.*;

public class Parser {
    private static class ParseError extends RuntimeException {}
    
    private final List<Token> _tokens;
    private int _current = 0;
    
    Parser(List<Token> tokens) {
        _tokens = tokens;
    }
    
    Expr parse() {
        try {
            return expression();
        } catch (ParseError error) {
            return null;
        }
    }
    
    private Expr expression() {
        return equality();
    }
    
    private Expr equality() {
        Expr expr = comparison();
        
        while (match(TK_NOTEQ, TK_ASSIGN)) {
            Token op = previous();
            Expr right = comparison();
            expr = new Expr.Binary(expr, op, right);
        }
        
        return expr;
    }
    
    private Expr comparison() {
        Expr expr = term();
        
        while (match(TK_GREATER, TK_GEQ, TK_LESS, TK_LEQ)) {
            Token op = previous();
            Expr right = term();
            expr = new Expr.Binary(expr, op, right);
        }
        
        return expr;
    }
    
    private Expr term() {
        Expr expr = factor();
        
        while (match(TK_MINUS, TK_PLUS)) {
            Token op = previous();
            Expr right = factor();
            expr = new Expr.Binary(expr, op, right);
        }
        
        return expr;
    }
    
    private Expr factor() {
        Expr expr = unary();
        
        while (match(TK_SLASH, TK_STAR)) {
            Token op = previous();
            Expr right = unary();
            expr = new Expr.Binary(expr, op, right);
        }
        
        return expr;
    }
    
    private Expr unary() {
        if (match(TK_NOT, TK_MINUS)) {
            Token op = previous();
            Expr right = unary();
            return new Expr.Unary(op, right);
        }
        
        return primary();
    }
    
    private Expr primary() {
        if (match(TK_FALSE)) {
            return new Expr.Literal(false);
        }
        
        if (match(TK_TRUE)) {
            return new Expr.Literal(true);
        }
        
        if (match(TK_NIL)) {
            return new Expr.Literal(null);
        }
        
        if (match(TK_NUMBER, TK_STRING)) {
            return new Expr.Literal(previous()._literal);
        }
        
        if (match(TK_LPAREN)) {
            Expr expr = expression();
            consume(TK_RPAREN, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }
        
        throw error(peek(), "Expected expression.");
    }
    
    private boolean match(TokenType... types) {
        for (TokenType type : types) {
            if (check(type)) {
                eat();
                return true;
            }
        }
        
        return false;
    }
    
    private Token consume(TokenType type, String msg) {
        if (check(type)) {
            return eat();
        }
        
        throw error(peek(), msg);
    }
    
    private boolean check(TokenType type) {
        if (atEnd()) {
            return false;
        }
        
        return peek()._type == type;
    }
    
    private Token eat() {
        if (!atEnd()) {
            ++_current;
        }
        
        return previous();
    }
    
    private boolean atEnd() {
        return peek()._type == TK_EOF;
    }
    
    private Token peek() {
        return _tokens.get(_current);
    }
    
    private Token previous() {
        return _tokens.get(_current - 1);
    }
    
    private ParseError error(Token token, String msg) {
        Main.error(token, msg);
        return new ParseError();
    }
    
    private void sync() {
        eat();
        
        while (!atEnd()) {
            if (previous()._type == TK_SEMICOLON) {
                return;
            }
            
            switch (peek()._type) {
                case TK_CLASS, TK_FN, TK_VAR, TK_FOR, TK_IF, TK_WHILE, TK_PRINT, TK_RETURN -> {return;}
            }
            
            eat();
        }
    }
}
