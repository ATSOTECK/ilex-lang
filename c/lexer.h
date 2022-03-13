//
// Created by Skyler on 3/12/22.
//

#ifndef __C_LEXER_H__
#define __C_LEXER_H__

typedef enum {
    // Single-character tokens.
    TK_LPAREN, // (
    TK_RPAREN, // )
    TK_LBRACE, // {
    TK_RBRACE, // }
    TK_COMMA, // ,
    TK_DOT, // .
    TK_MINUS, // -
    TK_PLUS, // +
    TK_SEMICOLON, // ;
    TK_DIV, // /
    TK_MUL, // *

    // One or two character tokens.
    TK_NOT, // !
    TK_NOTEQ, // !=
    TK_ASSIGN, // =
    TK_EQ, // ==
    TK_GR, // >
    TK_GREQ, // >=
    TK_LT, // <
    TK_LTEQ, // <=

    // Literals.
    TK_IDENT,
    TK_STRING,
    TK_NUMBER,

    // Keywords.
    TK_AND,
    TK_CLASS,
    TK_ELSE,
    TK_FALSE,
    TK_FOR,
    TK_FUN,
    TK_IF,
    TK_NULL,
    TK_OR,
    TK_PRINT,
    TK_RETURN,
    TK_SUPER,
    TK_THIS,
    TK_TRUE,
    TK_VAR,
    TK_WHILE,

    TK_ERROR,
    TK_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int len;
    int line;
} Token;

void initLexer(const char *source);
Token nextToken();

#endif //__C_LEXER_H__
