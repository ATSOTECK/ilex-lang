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
    TK_INC, // ++
    TK_DEC, // --
    TK_PLUSEQ, // +=
    TK_MINUSEQ, // -=
    TK_MULEQ, // *=
    TK_DIVEQ, // /=

    TK_VAR_DECL, // :=
    TK_SCOPE, // ::

    // Literals.
    TK_IDENT,
    TK_STRING,
    TK_NUMBER,

    // Keywords.
    TK_AND,
    TK_AS,
    TK_ASSERT,
    TK_BREAK, // TODO
    TK_CLASS,
    TK_CONST, // TODO
    TK_CONTINUE, // TODO
    TK_ELIF,
    TK_ELSE,
    TK_ENUM,
    TK_FALSE,
    TK_FOR,
    TK_FN,
    TK_GOTO, // TODO
    TK_IF,
    TK_NULL,
    TK_OR,
    TK_RETURN,
    TK_SUPER,
    TK_SWITCH, // TODO
    TK_THIS,
    TK_TRUE,
    TK_USE,
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
