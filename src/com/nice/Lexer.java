package com.nice;

import java.util.ArrayList;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.nice.TokenType.*;

public class Lexer {
    private final String _source;
    private final List<Token> _tokens = new ArrayList<Token>();
    private int _start = 0;
    private int _current = 0;
    private int _line = 1;
    
    private static final Map<String, TokenType> _keywords;
    
    static {
        _keywords = new HashMap<>();
        _keywords.put("and",    TK_AND);
        _keywords.put("class",  TK_CLASS);
        _keywords.put("else",   TK_ELSE);
        _keywords.put("elif",   TK_ELIF);
        _keywords.put("false",  TK_FALSE);
        _keywords.put("for",    TK_FOR);
        _keywords.put("fun",    TK_FN);
        _keywords.put("if",     TK_IF);
        _keywords.put("nil",    TK_NIL);
        _keywords.put("or",     TK_OR);
        _keywords.put("print",  TK_PRINT);
        _keywords.put("return", TK_RETURN);
        _keywords.put("super",  TK_SUPER);
        _keywords.put("this",   TK_THIS);
        _keywords.put("true",   TK_TRUE);
        _keywords.put("var",    TK_VAR);
        _keywords.put("while",  TK_WHILE);
    }
    
    Lexer(String source) {
        _source = source;
    }
    
    List<Token> lex() {
        while (!atEnd()) {
            _start = _current;
            nextToken();
        }
        
        _tokens.add(new Token(TK_EOF, "", null, _line));
        return _tokens;
    }
    
    private boolean atEnd() {
        return _current >= _source.length();
    }
    
    private void nextToken() {
        char c = eat();
    
        switch (c) {
            case '(' -> addToken(TK_LPAREN);
            case ')' -> addToken(TK_RPAREN);
            case '{' -> addToken(TK_LBRACE);
            case '}' -> addToken(TK_RBRACE);
            case ',' -> addToken(TK_COMMA);
            case '.' -> addToken(TK_DOT);
            case '-' -> addToken(TK_MINUS);
            case '+' -> addToken(TK_PLUS);
            case ';' -> addToken(TK_SEMICOLON);
            case '*' -> addToken(TK_STAR);
            case '!' -> addToken(match('=') ? TK_NOTEQ : TK_NOT);
            case '=' -> addToken(match('=') ? TK_EQ : TK_ASSIGN);
            case '<' -> addToken(match('=') ? TK_LEQ : TK_LESS);
            case '>' -> addToken(match('=') ? TK_GEQ : TK_GREATER);
            //TODO(Skyler): Block comments.
            case '/' -> {
                if (match('/')) {
                    while (peek() != '\n' && !atEnd()) {
                        eat();
                    }
                } else {
                    addToken(TK_SLASH);
                }
            }
            
            case ' ', '\r', '\t' -> {}
            case '\n' -> ++_line;
            case '"' -> string();
            
            default -> {
                if (isDigit(c)) {
                    number();
                } else if (isAlpha(c)) {
                    ident();
                } else {
                    Main.error(_line, "Unexpected character.");
                }
            }
        }
    }
    
    private void ident() {
        while (isAlphaNumeric(peek())) {
            eat();
        }
        
        String txt = _source.substring(_start, _current);
        TokenType type = _keywords.get(txt);
        
        if (type == null) {
            type = TK_IDENT;
        }
        
        addToken(type);
    }
    
    private void number() {
        while (isDigit(peek())) {
            eat();
        }
        
        if (peek() == '.' && isDigit(peekNext())) {
            eat(); // eat the '.'
            
            while (isDigit(peek())) {
                eat();
            }
        }
        
        addToken(TK_NUMBER, Double.parseDouble(_source.substring(_start, _current)));
    }
    
    private void string() {
        while (peek() != '"' && !atEnd()) {
            if (peek() == '\n') {
                ++_line;
            }
            
            eat();
        }
        
        if (atEnd()) {
            Main.error(_line, "Unterminated string.");
            return;
        }
        
        eat(); // eat the closing "
        
        String str = _source.substring(_start + 1, _current - 1);
        addToken(TK_STRING, str);
    }
    
    private boolean match(char expected) {
        if (atEnd()) {
            return false;
        }
        
        if (_source.charAt(_current) != expected) {
            return false;
        }
        
        ++_current;
        return true;
    }
    
    private char peek() {
        if (atEnd()) {
            return '\0';
        }
        
        return _source.charAt(_current);
    }
    
    private char peekNext() {
        if (_current + 1 >= _source.length()) {
            return '\0';
        }
        
        return _source.charAt(_current + 1);
    }
    
    private char eat() {
        return _source.charAt(_current++);
    }
    
    private void addToken(TokenType type) {
        addToken(type, null);
    }
    
    private void addToken(TokenType type, Object literal) {
        String txt = _source.substring(_start, _current);
        _tokens.add(new Token(type, txt, literal, _line));
    }
    
    private boolean isDigit(char c) {
        return (c >= '0' && c <= '9');
    }
    
    private boolean isAlpha(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }
    
    private boolean isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }
}
