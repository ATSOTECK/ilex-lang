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
    char previous;
    int line;
} Lexer;

Lexer lexer;

void initLexer(const char *source) {
    lexer.start = source;
    lexer.current = source;
    lexer.previous = '\0';
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

static Token makeToken(IlexTokenType type) {
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
    lexer.previous = *lexer.current;
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
            // Newlines are handled in nextToken().
            case '/':
                if (peekNext() == '/') {
                    while (peek() != '\n' && !atEnd()) {
                        advance();
                    }
                } else if (peekNext() == '-' || peekNext() == '*') {
                    int level = 0;
                    advance();
                    while (!atEnd() && (level >= 1 || !((peek() == '-' || peek() == '*') && peekNext() == '/'))) {
                        if (peek() == '\n') {
                            lexer.line++;
                        } else if (peek() == '/' && (peekNext() == '-' || peekNext() == '*')) {
                            ++level;
                        } else if ((peek() == '-' || peek() == '*') && peekNext() == '/') {
                            --level;
                        }

                        advance();
                    }

                    advance(); // - or *
                    advance();// /
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static IlexTokenType checkKeyword(int start, int len, const char *rest, IlexTokenType type) {
    if (lexer.current - lexer.start == start + len && memcmp(lexer.start + start, rest, len) == 0) {
        return type;
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

static IlexTokenType identType() {
    switch (lexer.start[0]) {
        case 'a':
            if (lexer.current - lexer.start > 1) {
                switch (lexer.start[1]) {
                    case 'n': return checkKeyword(2, 1, "d", TK_AND);
                    case 's':
                        if (lexer.start[2] == 's') {
                            return checkKeyword(2, 4, "sert", TK_ASSERT);
                        } else {
                            return TK_AS;
                        }
                }
            }
        case 'b': return checkKeyword(1, 4, "reak", TK_BREAK);
        case 'c':
            if (lexer.current - lexer.start > 1) {
                switch (lexer.start[1]) {
                    case 'a': return checkKeyword(2, 2, "se", TK_CASE);
                    case 'l': return checkKeyword(2, 3, "ass", TK_CLASS);
                    case 'o': {
                        IlexTokenType tk = checkKeyword(2, 3, "nst", TK_CONST);
                        if (tk == TK_IDENT) {
                            return checkKeyword(2, 6, "ntinue", TK_CONTINUE);
                        }
                        return tk;
                    }
                }
            }
        case 'd': return checkKeyword(1, 6, "efault", TK_DEFAULT);
        case 'e':
            if (lexer.current - lexer.start > 1) {
                switch (lexer.start[1]) {
                    case 'l':
                        if (lexer.current - lexer.start > 2) {
                            switch (lexer.start[2]) {
                                case 'i': return checkKeyword(3, 1, "f", TK_ELIF);
                                case 's': return checkKeyword(3, 1, "e", TK_ELSE);
                            }
                        }
                    case 'n': return checkKeyword(2, 2, "um", TK_ENUM);
                }
            }
        case 'f':
            if (lexer.current - lexer.start > 1) {
                switch (lexer.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TK_FALSE);
                    case 'n': return TK_FN;
                    case 'o': return checkKeyword(2, 1, "r", TK_FOR);
                    case 'r': return checkKeyword(2, 2, "om", TK_FROM);
                }
            }
        case 'i': return checkKeyword(1, 1, "f", TK_IF);
        case 'n': return checkKeyword(1, 3, "ull", TK_NULL);
        case 'o': return checkKeyword(1, 1, "r", TK_OR);
        case 'p': {
            int tk = checkKeyword(1, 4, "anic", TK_PANIC);
            if (tk == TK_PANIC && match('!')) {
                return tk;
            } else {
                return TK_IDENT;
            }
        }
        case 'r': return checkKeyword(1, 5, "eturn", TK_RETURN);
        case 's':
            if (lexer.current - lexer.start > 1) {
                switch (lexer.start[1]) {
                    case 'u': return checkKeyword(2, 3, "per", TK_SUPER);
                    case 'w': return checkKeyword(2, 4, "itch", TK_SWITCH);
                }
            }
        case 't':
            if (lexer.current - lexer.start > 1) {
                switch (lexer.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TK_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TK_TRUE);
                }
            } break;
        case 'u': return checkKeyword(1, 2, "se", TK_USE);
        case 'v': return checkKeyword(1, 2, "ar", TK_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TK_WHILE);
    }
    
    return TK_IDENT;
}

static Token string(char strChar) {
    bool overwrite = false;
    while ((peek() != strChar || overwrite) && !atEnd()) {
        overwrite = false;
        
        if (peek() == '\\' && peekNext() == strChar) {
            overwrite = true;
        }
        
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
        case '$': return ident();
        case '(': return makeToken(TK_LPAREN);
        case ')': return makeToken(TK_RPAREN);
        case '{': return makeToken(TK_LBRACE);
        case '}': return makeToken(TK_RBRACE);
        case '[': return makeToken(TK_LBRACKET);
        case ']': return makeToken(TK_RBRACKET);
        case ';': return makeToken(TK_SEMICOLON);
        case ',': return makeToken(TK_COMMA);
        case '.': return makeToken(TK_DOT);
        case '-': {
            if (match('-')) {
                return makeToken(TK_DEC);
            } else if (match('=')) {
                return makeToken(TK_MINUSEQ);
            } else if (match('>')) {
                return makeToken(TK_FALLTHROUGH);
            } else {
                return makeToken(TK_MINUS);
            }
        }
        case '+': {
            if (match('+')) {
                return makeToken(TK_INC);
            } else if (match('=')) {
                return makeToken(TK_PLUSEQ);
            } else {
                return makeToken(TK_PLUS);
            }
        }
        case '/': return makeToken(match('=') ? TK_DIVEQ : TK_DIV);
        case '*': {
            if (match('=')) {
                return makeToken(TK_MULEQ);
            } else if (match('*')) {
                return makeToken(match('=') ? TK_POWEQ : TK_POW);
            } else {
                return makeToken(TK_MUL);
            }
        }
        case '%': return makeToken(match('=') ? TK_MODEQ : TK_MOD);
        case '!': return makeToken(match('=') ? TK_NOTEQ : TK_NOT);
        case '=': return makeToken(match('=') ? TK_EQ    : TK_ASSIGN);
        case '<': {
            if (match('<')) {
                return makeToken(TK_BIT_LS);
            } else {
                return makeToken(match('=') ? TK_LTEQ : TK_LT);
            }
        }
        case '>': {
            if (match('>')) {
                return makeToken(TK_BIT_RS);
            } else {
                return makeToken(match('=') ? TK_GREQ : TK_GR);
            }
        }
        case ':': {
            if (match('=')) {
                return makeToken(TK_VAR_DECL);
            } else if (match(':')) {
                return makeToken(match('=') ? TK_CONST_DECL : TK_SCOPE);
            } else {
                return makeToken(TK_COLON);
            }
        }
        case '?': {
            if (match('.')) {
                return makeToken(TK_OPT);
            } else if (match('?')) {
                return makeToken(match('=') ? TK_NULL_COALESCE_EQ : TK_NULL_COALESCE);
            } else {
                return makeToken(TK_TER);
            }
        }
        case '&': {
            if (match('&')) {
                return makeToken(TK_AND);
            } else if (match('=')) {
                return makeToken(TK_BIT_ANDEQ);
            } else {
                return makeToken(TK_BIT_AND);
            }
        }
        case '|': {
            if (match('|')) {
                return makeToken(TK_OR);
            } else if (match('=')) {
                return makeToken(TK_BIT_OREQ);
            } else {
                return makeToken(TK_BIT_OR);
            }
        }
        case '^': {
            return makeToken(match('=') ? TK_BIT_XOREQ : TK_BIT_XOR);
        }
        case '~': return makeToken(TK_BIT_NOT);
        case '"': return string('"');
        case '\'': return string('\'');
        case '\n': {
            ++lexer.line;
            return makeToken(TK_NL);
        }
        default: break;
    }

    return errorToken("Unexpected character.");
}
