//
// Created by Skyler on 3/12/22.
//

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "lexer.h"

typedef struct {
    const char *start;
    const char *current;
    int line;
} Lexer;

Lexer lexer;

void initLexer(const char *source) {
    lexer.start = source;
    lexer.current = source;
    lexer.line = 1;
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static bool isAlphaNumeric(char c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') || (c >= '0' && c <= '9');
}

static bool atEnd() {
    return *lexer.current == '\0';
}

static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = lexer.start;
    token.len = (int)(lexer.current - lexer.start);
    token.line = lexer.line;

    return token;
}

static Token errorToken(const char* message) {
    Token token;
    token.type = TK_ERROR;
    token.start = message;
    token.len = (int)strlen(message);
    token.line = lexer.line;

    return token;
}

static char advance() {
    lexer.current++;
    return lexer.current[-1];
}

static char peek() {
    return *lexer.current;
}

static char peekNext() {
    if (atEnd()) {
        return '\0';
    }

    return lexer.current[1];
}

static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                lexer.line++;
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    while (peek() != '\n' && !atEnd()) {
                        advance();
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static TokenType checkKeyword(int start, int len, const char *rest, TokenType type) {
    if (lexer.current - lexer.start == start + len && memcmp(lexer.start + start, rest, len) == 0) {
        return type;
    }

    return TK_IDENT;
}

static TokenType identType() {
    switch (lexer.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TK_AND);
        case 'c': return checkKeyword(1, 4, "lass", TK_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TK_ELSE);
        case 'f':
            if (lexer.current - lexer.start > 1) {
                switch (lexer.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TK_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TK_FOR);
                    case 'u': return checkKeyword(2, 2, "nc", TK_FUN);
                }
            } break;
        case 'i': return checkKeyword(1, 1, "f", TK_IF);
        case 'n': return checkKeyword(1, 3, "ull", TK_NULL);
        case 'o': return checkKeyword(1, 1, "r", TK_OR);
        case 'p': return checkKeyword(1, 4, "rint", TK_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TK_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TK_SUPER);
        case 't':
            if (lexer.current - lexer.start > 1) {
                switch (lexer.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TK_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TK_TRUE);
                }
            } break;
        case 'v': return checkKeyword(1, 2, "ar", TK_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TK_WHILE);
    }
    
    return TK_IDENT;
}

static bool match(char expected) {
    if (atEnd()) {
        return false;
    }

    if (*lexer.current != expected) {
        return false;
    }
    lexer.current++;

    return true;
}

static Token string() {
    while (peek() != '"' && !atEnd()) {
        if (peek() == '\n') {
            lexer.line++;
        }

        advance();
    }

    if (atEnd()) {
        return errorToken("Unterminated string.");
    }

    advance();
    return makeToken(TK_STRING);
}

static Token ident() {
    while (isAlphaNumeric(peek())) {
        advance();
    }

    return makeToken(identType());
}

static Token number() {
    while (isDigit(peek())) {
        advance();
    }

    if (peek() == '.' && isDigit(peekNext())) {
        advance();

        while (isDigit(peek())) {
            advance();
        }
    }

    return makeToken(TK_NUMBER);
}

Token nextToken() {
    skipWhitespace();
    lexer.start = lexer.current;

    if (atEnd()) {
        return makeToken(TK_EOF);
    }

    char c = advance();
    if (isAlpha(c)) {
        return ident();
    }

    if (isDigit(c)) {
        return number();
    }

    switch (c) {
        case '(': return makeToken(TK_LPAREN);
        case ')': return makeToken(TK_RPAREN);
        case '{': return makeToken(TK_LBRACE);
        case '}': return makeToken(TK_RBRACE);
        case ';': return makeToken(TK_SEMICOLON);
        case ',': return makeToken(TK_COMMA);
        case '.': return makeToken(TK_DOT);
        case '-': return makeToken(TK_MINUS);
        case '+': return makeToken(TK_PLUS);
        case '/': return makeToken(TK_DIV);
        case '*': return makeToken(TK_MUL);
        case '!': return makeToken(match('=') ? TK_NOTEQ : TK_NOT);
        case '=': return makeToken(match('=') ? TK_EQ : TK_ASSIGN);
        case '<': return makeToken(match('=') ? TK_LTEQ : TK_LT);
        case '>': return makeToken(match('=') ? TK_GREQ : TK_GR);
        case '"': return string();
    }

    return errorToken("Unexpected character.");
}
