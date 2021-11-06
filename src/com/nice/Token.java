package com.nice;

public class Token {
    final TokenType _type;
    final String _lexeme;
    final Object _literal;
    final int _line;
    
    Token(TokenType type, String lexeme, Object literal, int line) {
        _type = type;
        _lexeme = lexeme;
        _literal = literal;
        _line = line;
    }
    
    public String toString() {
        return _type + " " + _lexeme + " " + _literal;
    }
}
