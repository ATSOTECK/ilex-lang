package com.nice;

public enum TokenType {
    TK_LPAREN,          // (
    TK_RPAREN,          // )
    TK_LBRACE,          // {
    TK_RBRACE,          // }
    TK_COMMA,           // ,
    TK_DOT,             // .
    TK_MINUS,           // -
    TK_PLUS,            // +
    TK_SEMICOLON,       // ;
    TK_SLASH,           // /
    TK_STAR,            // *
    
    TK_NOT,             // !
    TK_NOTEQ,           // !=
    TK_ASSIGN,          // =
    TK_EQ,              // ==
    TK_GREATER,         // >
    TK_GEQ,             // >=
    TK_LESS,            // <
    TK_LEQ,             // <=
    
    TK_IDENT,
    TK_STRING,
    TK_NUMBER,
    
    TK_AND,
    TK_CLASS,
    TK_ELSE,
    TK_ELIF,
    TK_FALSE,
    TK_FN,
    TK_FOR,
    TK_IF,
    TK_NIL,
    TK_OR,
    TK_PRINT,
    TK_RETURN,
    TK_SUPER,
    TK_THIS,
    TK_TRUE,
    TK_VAR,
    TK_WHILE,
    
    TK_EOF
}
