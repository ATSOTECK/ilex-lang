//
// Created by Skyler on 3/12/22.
//

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "lexer.h"
#include "memory.h"

#ifdef DEBUG_PRINT_CODE
#   include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    Token next;
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
    PREC_EXPONENT,    // **
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
    bool isConst;
    bool isCaptured;
} Local;

typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT
} FunctionType;

typedef struct Compiler {
    struct Compiler *enclosing;
    ObjFunction *function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;
} Compiler;

typedef struct ClassCompiler {
    struct ClassCompiler *enclosing;
    bool hasSuperclass;
} ClassCompiler;

Parser parser;
Compiler *current = NULL; // Will not work with multiple threads. This should be passed to the functions that need it.
ClassCompiler* currentClass = NULL;

static void expression();
static void statement();
static void declaration();
static void block();
static ParseRule* getRule(IlexTokenType type);
static void parsePrecedence(Precedence precedence);

static Chunk *currentChunk() {
    return &current->function->chunk;
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

static void error(const char *message, ...) {
    char *msg = (char*)malloc(sizeof(char) * 256);
    va_list args;
    va_start(args, message);
    vsnprintf(msg, 256, message, args);
    va_end(args);

    errorAt(&parser.previous, msg);
    free(msg);
}

static void errorAtCurrent(const char *message) {
    errorAt(&parser.current, message);
}

static void firstAdvance() {
    parser.next = nextToken();

    if (parser.next.type == TK_ERROR) {
        errorAtCurrent(parser.current.start);
    }
}

static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = parser.next;
        parser.next = nextToken();
        if (parser.current.type != TK_ERROR) {
            break;
        }

        errorAtCurrent(parser.current.start);
    }
}

static void eat(IlexTokenType type, const char *message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(IlexTokenType type) {
    return parser.current.type == type;
}

static bool lookahead(IlexTokenType type) {
    return parser.next.type == type;
}

static bool match(IlexTokenType type) {
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
    if (current->type == TYPE_INITIALIZER) {
        emitBytes(OP_GET_LOCAL, 0);  // this
    } else {
        emitByte(OP_NULL);
    }

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

static void initCompiler(Compiler *compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name = copyString(parser.previous.start, parser.previous.len);
    }

    Local *local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type != TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.len = 4;
    } else {
        local->name.start = "";
        local->name.len = 0;
    }
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

static int addUpvalue(Compiler *compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int  i = 0; i < upvalueCount; ++i) {
        Upvalue *upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;

    return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler *compiler, Token *name) {
    if (compiler->enclosing == NULL) {
        return -1;
    }

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
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
    local->isCaptured = false;
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

static uint8_t parseVariable(const char *errorMessage) {
    eat(TK_IDENT, errorMessage);
    declareVariable();

    if (current->scopeDepth > 0) {
        return 0;
    }

    return identifierConstant(&parser.previous);
}

static void defineVariable(uint8_t global, bool isConst) {
    if (current->scopeDepth > 0) {
        current->locals[current->localCount - 1].depth = current->scopeDepth;
        current->locals[current->localCount - 1].isConst = isConst;
        return;
    }

    if (isConst) {
        tableSet(&vm.consts, AS_STRING(currentChunk()->constants.values[global]), NULL_VAL);
    }
    emitBytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TK_RPAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(TK_COMMA));
    }

    eat(TK_RPAREN, "Expect ')' after arguments.");
    return argCount;
}

static ObjFunction *endCompiler() {
    emitReturn();
    ObjFunction *function = current->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL ? function->name->str : "<script>");
    }
#endif

    current = current->enclosing;
    return function;
}

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;

    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
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

static void orr(bool canAssgin) {
    // This means it is being used like x = val || otherVal
    IlexTokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));
    
    emitByte(OP_OR);
}

int parseEscapeSequences(char *str, int len) {
    int removeLen = 1;
    for (int i = 0; i < len - 1; i++) {
        if (str[i] == '\\') {
            switch (str[i + 1]) {
                case 'n':  str[i + 1] = '\n'; break;
                case 't':  str[i + 1] = '\t'; break;
                case 'r':  str[i + 1] = '\r'; break;
                case 'v':  str[i + 1] = '\v'; break;
                case '\\': str[i + 1] = '\\'; break;
                case '\'':
                case '"': break;
                case '0': {
                    if (str[i + 2] == '3' && str[i + 3] == '3') {
                        removeLen = 3;
                        str[i + removeLen] = '\033';
                    }
                } break;
                default:
                    continue;
            }
            
            memmove(&str[i], &str[i + removeLen], len - i);
            len -= removeLen;
        }
    }
    
    return len;
}

static Value parseString() {
    int strLen = parser.previous.len - 2;
    char *str = ALLOCATE(char, strLen + 1);
    
    memcpy(str, parser.previous.start + 1, strLen);
    int len = parseEscapeSequences(str, strLen);
    
    if (len != strLen) {
        str = SHRINK_ARRAY(str, char, strLen + 1, len + 1);
    }
    str[len] = '\0';
    
    return OBJ_VAL(takeString(str, len));
}

static void string(bool canAssign) {
    emitConstant(parseString());
}

static void checkIfConst(uint8_t setOp, int arg) {
    if (setOp == OP_SET_LOCAL) {
        if (current->locals[arg].isConst) {
            // TODO(Skyler): Find a better way to do this.
            char *name = (char*)malloc(current->locals[arg].name.len + 1);
            memcpy(name, current->locals[arg].name.start, current->locals[arg].name.len);
            error("Cannot assign to const variable '%s'.", name);
            free(name);
        }
    } else if (setOp == OP_SET_GLOBAL) {
        Value _;
        ObjString *name = AS_STRING(currentChunk()->constants.values[arg]);
        if (tableGet(&vm.consts, name, &_)) {
            error("Cannot assign to const variable '%s'.", name->str);
        }
    }
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);

    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

#define EMIT_OP_EQ(token) do { \
                              checkIfConst(setOp, arg); \
                              namedVariable(name, false); \
                              expression(); \
                              emitByte(token); \
                              emitBytes(setOp, (uint8_t)arg); \
                          } while (false) \
    
    if (canAssign && match(TK_ASSIGN)) {
        checkIfConst(setOp, arg);
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else if (canAssign && match(TK_PLUSEQ)) {
        EMIT_OP_EQ(OP_ADD);
    } else if (canAssign && match(TK_MINUSEQ)) {
        EMIT_OP_EQ(OP_SUB);
    } else if (canAssign && match(TK_MULEQ)) {
        EMIT_OP_EQ(OP_MUL);
    } else if (canAssign && match(TK_DIVEQ)) {
        EMIT_OP_EQ(OP_DIV);
    } else if (canAssign && match(TK_POWEQ)) {
        EMIT_OP_EQ(OP_POW);
    } else if (canAssign && match(TK_MODEQ)) {
        EMIT_OP_EQ(OP_MOD);
    } else if (canAssign && match(TK_NULL_COALESCE_EQ)) {
        EMIT_OP_EQ(OP_NULL_COALESCE);
    } else if (canAssign && match(TK_INC)) {
        checkIfConst(setOp, arg);
        namedVariable(name, false);
        emitByte(OP_INC);
        emitBytes(setOp, (uint8_t)arg);
    } else if (canAssign && match(TK_DEC)) {
        checkIfConst(setOp, arg);
        namedVariable(name, false);
        emitByte(OP_DEC);
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }

#undef EMIT_OP_EQ
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static Token syntheticToken(const char *str) {
    Token token;
    token.start = str;
    token.len = (int)strlen(str);

    return token;
}

static void super_(bool canAssign) {
    if (currentClass == NULL) {
        error("Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperclass) {
        error("Can't use 'super' in a class woth no superclass.");
    }

    eat(TK_DOT, "Expect '.' after 'super'.");
    eat(TK_IDENT, "Expect superclass method name.");
    uint8_t name = identifierConstant(&parser.previous);

    namedVariable(syntheticToken("this"), false);
    if (match(TK_LPAREN)) {
        uint8_t argc = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_SUPER_INVOKE, name);
        emitByte(argc);
    } else {
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_GET_SUPER, name);
    }
}

static void this_(bool canAssign) {
    if (currentClass == NULL) {
        error("Can't use 'this' outside of a class.");
        return;
    }

    variable(false);
}

static void binary(bool canAssign) {
    IlexTokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TK_NOTEQ:         emitByte(OP_NOTEQ); break;
        case TK_EQ:            emitByte(OP_EQ); break;
        case TK_GR:            emitByte(OP_GR); break;
        case TK_GREQ:          emitByte(OP_GREQ); break;
        case TK_LT:            emitByte(OP_LT); break;
        case TK_LTEQ:          emitByte(OP_LTEQ); break;
        case TK_PLUS:          emitByte(OP_ADD); break;
        case TK_MINUS:         emitByte(OP_SUB); break;
        case TK_MUL:           emitByte(OP_MUL); break;
        case TK_DIV:           emitByte(OP_DIV); break;
        case TK_POW:           emitByte(OP_POW); break;
        case TK_MOD:           emitByte(OP_MOD); break;
        case TK_NULL_COALESCE: emitByte(OP_NULL_COALESCE); break;
        default: return; // Unreachable.
    }
}

static void ternary(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    
    emitByte(OP_POP);
    expression();
    
    int endJump = emitJump(OP_JUMP);
    
    patchJump(elseJump);
    emitByte(OP_POP); // Condition.
    
    eat(TK_COLON, "Expect ':' after expression.");
    expression();
    
    patchJump(endJump);
}

static void call(bool canAssign) {
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void dot(bool canAssign) {
    eat(TK_IDENT, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&parser.previous);

#define EMIT_OP_EQ(token) do { \
                              emitBytes(OP_GET_PROPERTY_NO_POP, name); \
                              expression(); \
                              emitByte(token); \
                              emitBytes(OP_SET_PROPERTY, name); \
                          } while (false) \
    
    if (canAssign && match(TK_ASSIGN)) {
        expression();
        emitBytes(OP_SET_PROPERTY, name);
    } else if (canAssign && match(TK_PLUSEQ)) {
        EMIT_OP_EQ(OP_ADD);
    }  else if (canAssign && match(TK_MINUSEQ)) {
        EMIT_OP_EQ(OP_SUB);
    }  else if (canAssign && match(TK_MULEQ)) {
        EMIT_OP_EQ(OP_MUL);
    } else if (canAssign && match(TK_DIVEQ)) {
        EMIT_OP_EQ(OP_DIV);
    } else if (canAssign && match(TK_POWEQ)) {
        EMIT_OP_EQ(OP_POW);
    } else if (canAssign && match(TK_MODEQ)) {
        EMIT_OP_EQ(OP_MOD);
    } else if (canAssign && match(TK_NULL_COALESCE_EQ)) {
        EMIT_OP_EQ(OP_NULL_COALESCE);
    } else if (canAssign && match(TK_INC)) {
        emitBytes(OP_GET_PROPERTY_NO_POP, name);
        emitByte(OP_INC);
        emitBytes(OP_SET_PROPERTY, name);
    } else if (canAssign && match(TK_DEC)) {
        emitBytes(OP_GET_PROPERTY_NO_POP, name);
        emitByte(OP_DEC);
        emitBytes(OP_SET_PROPERTY, name);
    } else if (match(TK_LPAREN)) {
            uint8_t argCount = argumentList();
            emitBytes(OP_INVOKE, name);
            emitByte(argCount);
    } else {
        emitBytes(OP_GET_PROPERTY, name);
    }

#undef EMIT_OP_EQ
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
    IlexTokenType op = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch (op) {
        case TK_NOT: emitByte(OP_NOT); break;
        case TK_MINUS: emitByte(OP_NEG); break;
        default: return;
    }
}

static void inc(bool canAssign) {
    emitByte(OP_INC);
}

static void dec(bool canAssign) {
    emitByte(OP_DEC);
}

ParseRule rules[] = {
        [TK_LPAREN]           = {grouping, call,    PREC_CALL},
        [TK_RPAREN]           = {NULL,     NULL,    PREC_NONE},
        [TK_LBRACE]           = {NULL,     NULL,    PREC_NONE},
        [TK_RBRACE]           = {NULL,     NULL,    PREC_NONE},
        [TK_COMMA]            = {NULL,     NULL,    PREC_NONE},
        [TK_DOT]              = {NULL,     dot,     PREC_CALL},
        [TK_MINUS]            = {unary,    binary,  PREC_TERM},
        [TK_PLUS]             = {NULL,     binary,  PREC_TERM},
        [TK_PLUSEQ]           = {NULL,     NULL,    PREC_NONE},
        [TK_MINUSEQ]          = {NULL,     NULL,    PREC_NONE},
        [TK_MULEQ]            = {NULL,     NULL,    PREC_NONE},
        [TK_DIVEQ]            = {NULL,     NULL,    PREC_NONE},
        [TK_POWEQ]            = {NULL,     NULL,    PREC_NONE},
        [TK_POW]              = {NULL,     binary,  PREC_EXPONENT},
        [TK_MOD]              = {NULL,     binary,  PREC_FACTOR},
        [TK_MODEQ]            = {NULL,     NULL,    PREC_NONE},
        [TK_NULL_COALESCE]    = {NULL,     binary,  PREC_TERM},
        [TK_NULL_COALESCE_EQ] = {NULL,     NULL,    PREC_NONE},
        [TK_SEMICOLON]        = {NULL,     NULL,    PREC_NONE},
        [TK_DIV]              = {NULL,     binary,  PREC_FACTOR},
        [TK_MUL]              = {NULL,     binary,  PREC_FACTOR},
        [TK_NOT]              = {unary,    NULL,    PREC_NONE},
        [TK_NOTEQ]            = {NULL,     binary,  PREC_EQUALITY},
        [TK_ASSIGN]           = {NULL,     NULL,    PREC_NONE},
        [TK_EQ]               = {NULL,     binary,  PREC_EQUALITY},
        [TK_GR]               = {NULL,     binary,  PREC_COMPARISON},
        [TK_GREQ]             = {NULL,     binary,  PREC_COMPARISON},
        [TK_LT]               = {NULL,     binary,  PREC_COMPARISON},
        [TK_LTEQ]             = {NULL,     binary,  PREC_COMPARISON},
        [TK_IDENT]            = {variable, NULL,    PREC_NONE},
        [TK_STRING]           = {string,   NULL,    PREC_NONE},
        [TK_NUMBER]           = {number,   NULL,    PREC_NONE},
        [TK_AND]              = {NULL,     and_,    PREC_AND},
        [TK_CLASS]            = {NULL,     NULL,    PREC_NONE},
        [TK_ELSE]             = {NULL,     NULL,    PREC_NONE},
        [TK_FALSE]            = {literal,  NULL,    PREC_NONE},
        [TK_FOR]              = {NULL,     NULL,    PREC_NONE},
        [TK_FN]               = {NULL,     NULL,    PREC_NONE},
        [TK_IF]               = {NULL,     NULL,    PREC_NONE},
        [TK_NULL]             = {literal,  NULL,    PREC_NONE},
        [TK_OR]               = {NULL,     or_,     PREC_OR},
        [TK_RETURN]           = {NULL,     NULL,    PREC_NONE},
        [TK_SUPER]            = {super_,   NULL,    PREC_NONE},
        [TK_THIS]             = {this_,    NULL,    PREC_NONE},
        [TK_TRUE]             = {literal,  NULL,    PREC_NONE},
        [TK_VAR]              = {NULL,     NULL,    PREC_NONE},
        [TK_CONST]            = {NULL,     NULL,    PREC_NONE},
        [TK_VAR_DECL]         = {NULL,     NULL,    PREC_NONE},
        [TK_WHILE]            = {NULL,     NULL,    PREC_NONE},
        [TK_ASSERT]           = {NULL,     NULL,    PREC_NONE},
        [TK_INC]              = {NULL,     inc,     PREC_TERM},
        [TK_DEC]              = {NULL,     dec,     PREC_TERM},
        [TK_TER]              = {NULL,     ternary, PREC_ASSIGN},
        [TK_ERROR]            = {NULL,     NULL,    PREC_NONE},
        [TK_EOF]              = {NULL,     NULL,    PREC_NONE},
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
        bool setInfixToOrr = false;
        if (parser.current.type == TK_OR && precedence == PREC_ASSIGN) {
            setInfixToOrr = true;
        }
        
        advance();
        
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        if (setInfixToOrr) {
            infixRule = orr;
        }
        
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
            case TK_FN:
            case TK_VAR:
            case TK_CONST:
            case TK_FOR:
            case TK_IF:
            case TK_WHILE:
            case TK_RETURN:
            case TK_ASSERT:
                return;

            default:
                ; // Do nothing.
        }

        advance();
    }
}

static ParseRule* getRule(IlexTokenType type) {
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

static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    eat(TK_LPAREN, "Expect '(' after function name.");
    if (!check(TK_RPAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }

            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant, false); // TODO: Should this be true?
        } while (match(TK_COMMA));
    }
    eat(TK_RPAREN, "Expect ')' after parameters.");
    eat(TK_LBRACE, "Expect '{' before function body.");
    block();

    ObjFunction *function = endCompiler();
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; ++i) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

static void method() {
    eat(TK_IDENT, "Expect method name.");
    uint8_t constant = identifierConstant(&parser.previous);

    FunctionType type = TYPE_METHOD;
    if (parser.previous.len == 4 &&
        memcmp(parser.previous.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }
    function(type);
    emitBytes(OP_METHOD, constant);
}

static void classDeclaration() {
    eat(TK_IDENT, "Expect class name.");
    Token className = parser.previous;
    uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();

    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant, false);

    ClassCompiler classCompiler;
    classCompiler.enclosing = currentClass;
    classCompiler.hasSuperclass = false;
    currentClass = &classCompiler;

    if (match(TK_LT)) {
        eat(TK_IDENT, "Expect superclass name.");
        variable(false);

        if (identifiersEqual(&className, &parser.previous)) {
            error("A class can't inherit from itself.");
        }

        beginScope();
        addLocal(syntheticToken("super"));
        defineVariable(0, false);

        namedVariable(className, false);
        emitByte(OP_INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(className, false);
    eat(TK_LBRACE, "Expect '{' before class body.");
    while (!check(TK_RBRACE) && !check(TK_EOF)) {
        method();
    }
    eat(TK_RBRACE, "Expect '}' after class body.");
    emitByte(OP_POP);

    if (classCompiler.hasSuperclass) {
        endScope();
    }

    currentClass = currentClass->enclosing;
}

static void fnDeclaration() {
    uint8_t global = parseVariable("Expect function name.");
    //markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global, false);
}

static void varDeclaration(bool isConst) {
    do {
        uint8_t global = parseVariable("Expect variable name.");

        if (match(TK_ASSIGN) || isConst) {
            expression();
        } else {
            emitByte(OP_NULL);
        }

        defineVariable(global, isConst);
    } while (match(TK_COMMA));

    match(TK_SEMICOLON);
}

static void varDeclaration2() {
    eat(TK_IDENT, "Expect variable name.");
    declareVariable();
    uint8_t global;

    if (current->scopeDepth > 0) {
        global = 0;
    } else {
        global = identifierConstant(&parser.previous);
    }

    eat(TK_VAR_DECL, "Expected := after variable name.");
    expression();
    defineVariable(global, false);

    match(TK_SEMICOLON);
}

static void expressionStatement() {
    expression();
    match(TK_SEMICOLON);
    emitByte(OP_POP);
}

static void forStatement() {
    beginScope();
    eat(TK_LPAREN, "Expect '(' after 'for'.");

    bool initializer = true;
    if (match(TK_SEMICOLON)) {
        initializer = false;
    } else if (match(TK_VAR)) {
        varDeclaration(false);
    } else if (check(TK_IDENT) && lookahead(TK_VAR_DECL)) {
        varDeclaration2();
    } else {
        expressionStatement();
    }

    if (initializer) {
        if (parser.previous.type != TK_SEMICOLON) {
            errorAtCurrent("Expect ';' after loop initializer.");
        }
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

static void assertStatement() {
    eat(TK_LPAREN, "Expect '(' after 'assert'.");

    int constant = addConstant(currentChunk(), OBJ_VAL(copyString("", 0)));

    expression();

    if (match(TK_COMMA)) {
        eat(TK_STRING, "Expect assert error string after ','.");
        constant = addConstant(currentChunk(), OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.len - 2)));
    }
    eat(TK_RPAREN, "Expect ')' after condition.");
    match(TK_SEMICOLON);

    emitBytes(OP_ASSERT, (uint8_t)constant);
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

    if (match(TK_ELIF)) {
        ifStatement();
    } else if (match(TK_ELSE)) {
        if (match(TK_IF)) {
            ifStatement();
        } else {
            eat(TK_LBRACE, "Expect '{' after 'else'.");
            beginScope();
            block();
            endScope();
        }
    }
    patchJump(elseJump);
}

static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if (match(TK_SEMICOLON)) {
        emitReturn();
    } else {
        if (current->type == TYPE_INITIALIZER) {
            error("Can't return a value from an initializer.");
        }

        expression();
        match(TK_SEMICOLON);
        emitByte(OP_RETURN);
    }
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
    if (match(TK_CLASS)) {
        classDeclaration();
    } else if (match(TK_FN)) {
        fnDeclaration();
    } else if (match(TK_VAR)) {
        varDeclaration(false);
    } else if (check(TK_IDENT) && lookahead(TK_VAR_DECL)) {
        varDeclaration2();
    } else if (match(TK_CONST)) {
        varDeclaration(true);
    } else {
        statement();
    }

    if (parser.panicMode) {
        synchronize();
    }
}

static void statement() {
    if (match(TK_FOR)) {
        forStatement();
    } else if (match(TK_IF)) {
        ifStatement();
    } else if (match(TK_RETURN)) {
        returnStatement();
    } else if (match(TK_WHILE)) {
        whileStatement();
    } else if (match(TK_ASSERT)) {
        assertStatement();
    } else if (match(TK_LBRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

ObjFunction *compile(const char *source) {
    initLexer(source);
    Compiler compiler;
    firstAdvance();
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    while (!match(TK_EOF)) {
        declaration();
    }

    ObjFunction *function = endCompiler();
    return parser.hadError ? NULL : function;
}

void markCompilerRoots() {
    Compiler *compiler = current;

    while (compiler != NULL) {
        markObject((Obj*)compiler->function);
        compiler = compiler->enclosing;
    }
}
