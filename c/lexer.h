//
// Created by Skyler on 3/12/22.
//

#ifndef __C_LEXER_H__
#define __C_LEXER_H__

#define MAX_INTERPOLATION_DEPTH 2

typedef enum {
    // Single-character tokens.
    TK_LPAREN,           // (
    TK_RPAREN,           // )
    TK_LBRACE,           // {
    TK_RBRACE,           // }
    TK_LBRACKET,         // [
    TK_RBRACKET,         // ]
    TK_COMMA,            // ,
    TK_DOT,              // .
    TK_MINUS,            // -
    TK_PLUS,             // +
    TK_SEMICOLON,        // ;
    TK_COLON,            // :
    TK_DIV,              // /
    TK_MUL,              // *
    TK_TER,              // ?
    TK_HASH,             // #

    // One or two character tokens.
    TK_NOT,              // !
    TK_NOTEQ,            // !=
    TK_ASSIGN,           // =
    TK_EQ,               // ==
    TK_GR,               // >
    TK_GREQ,             // >=
    TK_LT,               // <
    TK_LTEQ,             // <=
    TK_INC,              // ++
    TK_DEC,              // --
    TK_PLUSEQ,           // +=
    TK_MINUSEQ,          // -=
    TK_MULEQ,            // *=
    TK_DIVEQ,            // /=
    TK_POW,              // **
    TK_POWEQ,            // **=
    TK_MOD,              // %
    TK_MODEQ,            // %=
    TK_BIT_AND,          // &
    TK_BIT_ANDEQ,        // &=
    TK_BIT_OR,           // |
    TK_BIT_OREQ,         // |=
    TK_BIT_XOR,          // ^
    TK_BIT_XOREQ,        // ^=
    TK_BIT_NOT,          // ~
    TK_BIT_LS,           // <<
    TK_BIT_RS,           // >>
    TK_OPT,              // ?.
    TK_NULL_COALESCE,    // ??
    TK_NULL_COALESCE_EQ, // ??=
    TK_ARROW,            // ->

    TK_VAR_DECL,         // :=
    TK_CONST_DECL,       // ::=
    TK_SCOPE,            // ::

    // Literals.
    TK_IDENT,
    TK_STRING,
    TK_INTERPOLATION,
    TK_NUMBER,

    // Keywords.
    TK_ABSTRACT,
    TK_AND,
    TK_AS,
    TK_ASSERT,
    TK_BEHAVIOR,
    TK_BREAK,
    TK_CASE,
    TK_CLASS,
    TK_CONST,
    TK_CONTINUE,
    TK_DEFAULT,
    TK_DO,
    TK_DOES,
    TK_ELIF,
    TK_ELSE,
    TK_ENUM,
    TK_FALSE,
    TK_FINAL,
    TK_FN,
    TK_FOR,
    TK_FROM,
    TK_GOTO, // TODO
    TK_IF,
    TK_IN,
    TK_IS,
    TK_INHERITS,
    TK_NAMESPACE,
    TK_NULL,
    TK_OR,
    TK_PANIC,
    TK_PRIVATE,
    TK_PUBLIC,
    TK_RETURN,
    TK_SUPER,
    TK_SWITCH,
    TK_STATIC,
    TK_THIS,
    TK_TRUE,
    TK_UNTIL,
    TK_USE,
    TK_VAR,
    TK_WHEN,
    TK_WHILE,
    TK_WITH_FILE,

    // No character representation.
    TK_TO_STR,

    TK_ERROR,
    TK_EOF,
} IlexTokenType;

typedef struct {
    IlexTokenType type;
    const char *start;
    int len;
    int line;
} Token;

void initLexer(const char *source);
Token nextToken();

#endif //__C_LEXER_H__
