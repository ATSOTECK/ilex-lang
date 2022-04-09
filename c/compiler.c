//
// Created by Skyler on 3/12/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "lexer.h"

#ifdef DEBUG_PRINT_CODE
#   include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGN,      // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth;
} Local;

typedef struct {
    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
} Compiler;

Parser parser;
Compiler *current = NULL; // Will not work with multiple threads. This should be passed to the functions that need it.
Chunk *compilingChunk;

static void expression();
static void statement();
static void declaration();
static void block();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static Chunk *currentChunk() {
    return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) {
        return;
    }

    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TK_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TK_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->len, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char *message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char *message) {
    errorAt(&parser.current, message);
}

static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = nextToken();
        if (parser.current.type != TK_ERROR) {
            break;
        }

        errorAtCurrent(parser.current.start);
    }
}

static void eat(TokenType type, const char *message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();

    return true;
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        error("Loop body too large.");
    }

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);

    return currentChunk()->count - 2;
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void patchJump(int offset) {
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

static void initCompiler(Compiler *compiler) {
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}

static uint8_t identifierConstant(Token *name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->len)));
}

static bool identifiersEqual(Token *a, Token *b) {
    if (a->len != b->len) {
        return false;
    }

    return memcmp(a->start, b->start, a->len) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("To many local variables in function.");
        return;
    }

    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
}

static void declareVariable() {
    if (current->scopeDepth == 0) {
        return;
    }

    Token *name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

static void endCompiler() {
    emitReturn();

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;

    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        emitByte(OP_POP);
        current->localCount--;
    }
}

static void grouping(bool canAssign) {
    expression();
    eat(TK_RPAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
    double val = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(val));
}

static void and_(bool canAssign) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

static void or_(bool canAssign) {
    // TODO: Jump if true.
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.len - 2)));
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);

    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TK_ASSIGN)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TK_NOTEQ:       emitByte(OP_NOTEQ); break;
        case TK_EQ:          emitByte(OP_EQ); break;
        case TK_GR:          emitByte(OP_GR); break;
        case TK_GREQ:        emitByte(OP_GREQ); break;
        case TK_LT:          emitByte(OP_LT); break;
        case TK_LTEQ:        emitByte(OP_LTEQ); break;
        case TK_PLUS:        emitByte(OP_ADD); break;
        case TK_MINUS:       emitByte(OP_SUB); break;
        case TK_MUL:         emitByte(OP_MUL); break;
        case TK_DIV:         emitByte(OP_DIV); break;
        default: return; // Unreachable.
    }
}

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TK_FALSE: emitByte(OP_FALSE); break;
        case TK_NULL:  emitByte(OP_NULL); break;
        case TK_TRUE:  emitByte(OP_TRUE); break;
        default: return; // Unreachable.
    }
}

static void unary(bool canAssign) {
    TokenType op = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch (op) {
        case TK_NOT: emitByte(OP_NOT); break;
        case TK_MINUS: emitByte(OP_NEG); break;
        default: return;
    }
}

ParseRule rules[] = {
        [TK_LPAREN]        = {grouping, NULL,   PREC_NONE},
        [TK_RPAREN]        = {NULL,     NULL,   PREC_NONE},
        [TK_LBRACE]        = {NULL,     NULL,   PREC_NONE},
        [TK_RBRACE]        = {NULL,     NULL,   PREC_NONE},
        [TK_COMMA]         = {NULL,     NULL,   PREC_NONE},
        [TK_DOT]           = {NULL,     NULL,   PREC_NONE},
        [TK_MINUS]         = {unary,    binary, PREC_TERM},
        [TK_PLUS]          = {NULL,     binary, PREC_TERM},
        [TK_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
        [TK_DIV]           = {NULL,     binary, PREC_FACTOR},
        [TK_MUL]           = {NULL,     binary, PREC_FACTOR},
        [TK_NOT]           = {unary,    NULL,   PREC_NONE},
        [TK_NOTEQ]         = {NULL,     binary, PREC_EQUALITY},
        [TK_ASSIGN]        = {NULL,     NULL,   PREC_NONE},
        [TK_EQ]            = {NULL,     binary, PREC_EQUALITY},
        [TK_GR]            = {NULL,     binary, PREC_COMPARISON},
        [TK_GREQ]          = {NULL,     binary, PREC_COMPARISON},
        [TK_LT]            = {NULL,     binary, PREC_COMPARISON},
        [TK_LTEQ]          = {NULL,     binary, PREC_COMPARISON},
        [TK_IDENT]         = {variable, NULL,   PREC_NONE},
        [TK_STRING]        = {string,   NULL,   PREC_NONE},
        [TK_NUMBER]        = {number,   NULL,   PREC_NONE},
        [TK_AND]           = {NULL,     and_,   PREC_AND},
        [TK_CLASS]         = {NULL,     NULL,   PREC_NONE},
        [TK_ELSE]          = {NULL,     NULL,   PREC_NONE},
        [TK_FALSE]         = {literal,  NULL,   PREC_NONE},
        [TK_FOR]           = {NULL,     NULL,   PREC_NONE},
        [TK_FUN]           = {NULL,     NULL,   PREC_NONE},
        [TK_IF]            = {NULL,     NULL,   PREC_NONE},
        [TK_NULL]          = {literal,  NULL,   PREC_NONE},
        [TK_OR]            = {NULL,     or_,    PREC_OR},
        [TK_PRINT]         = {NULL,     NULL,   PREC_NONE},
        [TK_RETURN]        = {NULL,     NULL,   PREC_NONE},
        [TK_SUPER]         = {NULL,     NULL,   PREC_NONE},
        [TK_THIS]          = {NULL,     NULL,   PREC_NONE},
        [TK_TRUE]          = {literal,  NULL,   PREC_NONE},
        [TK_VAR]           = {NULL,     NULL,   PREC_NONE},
        [TK_WHILE]         = {NULL,     NULL,   PREC_NONE},
        [TK_ERROR]         = {NULL,     NULL,   PREC_NONE},
        [TK_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGN;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TK_ASSIGN)) {
        error("Invalid assignment target.");
    }
}

static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TK_EOF) {
        if (parser.previous.type == TK_SEMICOLON) {
            return;
        }
        switch (parser.current.type) {
            case TK_CLASS:
            case TK_FUN:
            case TK_VAR:
            case TK_FOR:
            case TK_IF:
            case TK_WHILE:
            case TK_PRINT:
            case TK_RETURN:
                return;

            default:
                ; // Do nothing.
        }

        advance();
    }
}

static uint8_t parseVariable(const char *errorMessage) {
    eat(TK_IDENT, errorMessage);
    declareVariable();

    if (current->scopeDepth > 0) {
        return 0;
    }

    return identifierConstant(&parser.previous);
}

static void markInitialized() {
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void expression() {
    parsePrecedence(PREC_ASSIGN);
}

static void block() {
    while (!check(TK_RBRACE) && !check(TK_EOF)) {
        declaration();
    }

    eat(TK_RBRACE, "Expect '}' after block.");
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TK_ASSIGN)) {
        expression();
    } else {
        emitByte(OP_NULL);
    }
    eat(TK_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void expressionStatement() {
    expression();
    eat(TK_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void forStatement() {
    beginScope();
    eat(TK_LPAREN, "Expect '(' after 'for'.");

    if (match(TK_SEMICOLON)) {
        // No initializer.
    } else if (match(TK_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if (!match(TK_SEMICOLON)) {
        expression();
        eat(TK_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Condition.
    }

    if (!match(TK_RPAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        eat(TK_RPAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    eat(TK_LBRACE, "Expect '{' after ')'.");
    beginScope();
    block();
    endScope();

    emitLoop(loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP); // Condition.
    }

    endScope();
}

static void ifStatement() {
    eat(TK_LPAREN, "Expect '(' after 'if'.");
    expression();
    eat(TK_RPAREN, "Expect ')' after condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    eat(TK_LBRACE, "Expect '{' after ')'.");
    beginScope();
    block();
    endScope();

    int elseJump = emitJump(OP_JUMP);
    patchJump(thenJump);
    emitByte(OP_POP);

    if (match(TK_ELSE)) {
        eat(TK_LBRACE, "Expect '{' after 'else'.");
        beginScope();
        block();
        endScope();
    }
    patchJump(elseJump);
}

static void printStatement() {
    expression();
    eat(TK_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void whileStatement() {
    int loopStart = currentChunk()->count;
    eat(TK_LPAREN, "Expect '(' after 'while'.");
    expression();
    eat(TK_RPAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    eat(TK_LBRACE, "Expect '{' after ').");
    beginScope();
    block();
    endScope();
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);
}

static void declaration() {
    if (match(TK_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode) {
        synchronize();
    }
}

static void statement() {
    if (match(TK_PRINT)) {
        printStatement();
    } else if (match(TK_FOR)) {
        forStatement();
    } else if (match(TK_IF)) {
        ifStatement();
    } else if (match(TK_WHILE)) {
        whileStatement();
    } else if (match(TK_LBRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

bool compile(const char *source, Chunk *chunk) {
    initLexer(source);
    Compiler compiler;
    initCompiler(&compiler);
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    while (!match(TK_EOF)) {
        declaration();
    }

    endCompiler();
    return !parser.hadError;
}

