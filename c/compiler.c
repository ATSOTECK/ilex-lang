//
// Created by Skyler on 3/12/22.
//

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libs/lib_builtIn.h"
#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_PRINT_CODE
#   include "debug.h"
#endif

#define LOCAL_COUNT 10000

typedef enum {
    PREC_NONE,
    PREC_ASSIGN,      // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_BIT_OR,      // |
    PREC_BIT_XOR,     // ^
    PREC_BIT_AND,     // &
    PREC_BIT_SHIFT,   //<< >>
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_EXPONENT,    // **
    PREC_CALL,        // . () []
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(Compiler *compiler, bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

static void expression(Compiler *compiler);
static void statement(Compiler *compiler);
static void declaration(Compiler *compiler);
static void block(Compiler *compiler);
static ParseRule* getRule(IlexTokenType type);
static void parsePrecedence(Compiler *compiler, Precedence precedence);

static Chunk *currentChunk(Compiler *compiler) {
    return &compiler->function->chunk;
}

static void errorAt(Parser *parser, Token* token, const char* message) {
    if (parser->panicMode) {
        return;
    }

    parser->panicMode = true;
    fprintf(stderr, "%s [line %d] \033[31mError\033[m", parser->script->name->str, token->line);

    if (token->type == TK_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TK_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->len, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser->hadError = true;
}

static void error(Parser *parser, const char *message, ...) {
    char *msg = (char*)malloc(sizeof(char) * 256);
    va_list args;
    va_start(args, message);
    vsnprintf(msg, 256, message, args);
    va_end(args);

    errorAt(parser, &parser->previous, msg);
    free(msg);
}

static void errorAtCurrent(Parser *parser, const char *message) {
    errorAt(parser, &parser->current, message);
}

static void firstAdvance(Compiler *compiler) {
    compiler->parser->next = nextToken();

    if (compiler->parser->next.type == TK_ERROR) {
        errorAtCurrent(compiler->parser, compiler->parser->current.start);
    }
}

static void advance(Parser *parser) {
    parser->previous = parser->current;

    for (;;) {
        parser->current = parser->next;
        parser->next = nextToken();
        if (parser->current.type != TK_ERROR) {
            break;
        }

        errorAtCurrent(parser, parser->current.start);
    }
}

static void eat(Parser *parser, IlexTokenType type, const char *message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }

    errorAtCurrent(parser, message);
}

static bool check(Compiler *compiler, IlexTokenType type) {
    return compiler->parser->current.type == type;
}

static bool lookahead(Compiler *compiler, IlexTokenType type) {
    return compiler->parser->next.type == type;
}

static bool match(Compiler *compiler, IlexTokenType type) {
    if (!check(compiler, type)) {
        return false;
    }
    advance(compiler->parser);

    return true;
}

static void emitByte(Compiler *compiler, uint8_t byte) {
    writeChunk(compiler->parser->vm, currentChunk(compiler), byte, compiler->parser->previous.line);
}

static void emitShort(Compiler *compiler, uint16_t byte) {
    emitByte(compiler, (uint8_t)(byte >> 8));
    emitByte(compiler, (uint8_t)byte);
}

static void emitBytes(Compiler *compiler, uint8_t byte1, uint8_t byte2) {
    emitByte(compiler, byte1);
    emitByte(compiler, byte2);
}

static void emitByteShort(Compiler *compiler, uint8_t byte1, uint16_t byte2) {
    emitByte(compiler, byte1);
    emitShort(compiler, byte2);
}

static void emitShortByte(Compiler *compiler, uint16_t byte1, uint8_t byte2) {
    emitShort(compiler, byte1);
    emitByte(compiler, byte2);
}

static void emitLoop(Compiler *compiler, int loopStart) {
    emitByte(compiler, OP_LOOP);

    int offset = currentChunk(compiler)->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        error(compiler->parser, "Loop body too large.");
    }

    emitByte(compiler, (offset >> 8) & 0xff);
    emitByte(compiler, offset & 0xff);
}

static int emitJump(Compiler *compiler, uint8_t instruction) {
    emitByte(compiler, instruction);
    emitByte(compiler, 0xff);
    emitByte(compiler, 0xff);

    return currentChunk(compiler)->count - 2;
}

static void emitReturn(Compiler *compiler) {
    if (compiler->type == TYPE_INITIALIZER) {
        emitByteShort(compiler, OP_GET_LOCAL, 0);  // this
    } else {
        emitByte(compiler, OP_NULL);
    }

    emitByte(compiler, OP_RETURN);
}

static uint16_t makeConstant(Compiler *compiler, Value value) {
    int constant = addConstant(compiler->parser->vm, currentChunk(compiler), value);
    if (constant > UINT16_MAX) {
        error(compiler->parser, "Too many constants in one chunk.");
        return 0;
    }

    return (uint16_t)constant;
}

static void emitConstant(Compiler *compiler, Value value) {
    emitByteShort(compiler, OP_CONSTANT, makeConstant(compiler, value));
}

static void patchJump(Compiler *compiler, int offset) {
    int jump = currentChunk(compiler)->count - offset - 2;

    if (jump > UINT16_MAX) {
        error(compiler->parser, "Too much code to jump over.");
    }

    currentChunk(compiler)->code[offset] = (jump >> 8) & 0xff;
    currentChunk(compiler)->code[offset + 1] = jump & 0xff;
}

static void initCompiler(Parser *parser, Compiler *compiler, Compiler *parent, FunctionType type) {
    compiler->parser = parser;
    compiler->enclosing = parent;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction(parser->vm, parser->script);
    compiler->currentLibName = 0;
    compiler->currentScript = NULL;
    compiler->loop = NULL;
    compiler->locals = NULL;
    compiler->upvalues = NULL;
    compiler->isWithBlock = false;
    compiler->withVarName = NULL;

    compiler->locals   = (Local*)  malloc(sizeof(Local)   * LOCAL_COUNT);
    compiler->upvalues = (Upvalue*)malloc(sizeof(Upvalue) * LOCAL_COUNT);

    if (parent != NULL) {
        compiler->class = parent->class;
        compiler->loop = parent->loop;
    }

    parser->vm->compiler = compiler;

    if (type == TYPE_ANON) {
        compiler->function->name = copyString(parser->vm, "<anonymous>", 11);
    } else if (type != TYPE_SCRIPT) {
        compiler->function->name = copyString(parser->vm, parser->previous.start, parser->previous.len);
    } else {
        compiler->function->name = NULL;
    }

    Local *local = &compiler->locals[compiler->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type == TYPE_METHOD || type == TYPE_INITIALIZER) {
        local->name.start = "this";
        local->name.len = 4;
    } else {
        local->name.start = "";
        local->name.len = 0;
    }
}

static uint16_t identifierConstant(Compiler *compiler, Token *name) {
    return makeConstant(compiler, OBJ_VAL(copyString(compiler->parser->vm, name->start, name->len)));
}

static bool identifiersEqual(Token *a, Token *b) {
    if (a->len != b->len) {
        return false;
    }

    return memcmp(a->start, b->start, a->len) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name, bool inFunction) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (!inFunction && local->depth == -1) {
                error(compiler->parser, "Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static int addUpvalue(Compiler *compiler, uint16_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int  i = 0; i < upvalueCount; ++i) {
        Upvalue *upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == LOCAL_COUNT) {
        error(compiler->parser, "Too many closure variables in function.");
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

    int local = resolveLocal(compiler->enclosing, name, true);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint16_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint16_t)upvalue, false);
    }

    return -1;
}

static void addLocal(Compiler *compiler, Token name) {
    if (compiler->localCount == LOCAL_COUNT) {
        error(compiler->parser, "To many local variables in function.");
        return;
    }

    Local *local = &compiler->locals[compiler->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}

static void declareVariable(Compiler *compiler) {
    if (compiler->scopeDepth == 0) {
        return;
    }

    Token *name = &compiler->parser->previous;
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (local->depth != -1 && local->depth < compiler->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error(compiler->parser, "Already a variable with this name in this scope.");
        }
    }

    addLocal(compiler, *name);
}

static uint16_t parseVariable(Compiler *compiler, const char *errorMessage) {
    eat(compiler->parser, TK_IDENT, errorMessage);
    declareVariable(compiler);

    if (compiler->scopeDepth > 0) {
        return 0;
    }

    return identifierConstant(compiler, &compiler->parser->previous);
}

static void defineVariable(Compiler *compiler, uint16_t global, bool isConst) {
    if (compiler->scopeDepth == 0) {
        if (isConst) {
            tableSet(compiler->parser->vm, &compiler->parser->vm->consts, AS_STRING(currentChunk(compiler)->constants.values[global]), NULL_VAL);
        }
    
        emitByteShort(compiler, OP_DEFINE_SCRIPT, global);
    } else {
        compiler->locals[compiler->localCount - 1].depth = compiler->scopeDepth;
        compiler->locals[compiler->localCount - 1].isConst = isConst;

        return;
    }
}

static uint8_t argumentList(Compiler *compiler) {
    uint8_t argCount = 0;
    if (!check(compiler, TK_RPAREN)) {
        do {
            expression(compiler);
            if (argCount > 255) {
                error(compiler->parser, "Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(compiler, TK_COMMA));
    }
    
    eat(compiler->parser, TK_RPAREN, "Expect ')' after arguments.");
    return argCount;
}

static ObjFunction *endCompiler(Compiler *compiler) {
    emitReturn(compiler);
    ObjFunction *function = compiler->function;

#ifdef DEBUG_PRINT_CODE
    if (!compiler->parser->hadError) {
        disassembleChunk(currentChunk(compiler), function->name != NULL ? function->name->str : function->script->name->str);
    }
#endif

    compiler->parser->vm->compiler = compiler->enclosing;
    return function;
}

static void beginScope(Compiler *compiler) {
    compiler->scopeDepth++;
}

static void endScope(Compiler *compiler) {
    compiler->scopeDepth--;

    while (compiler->localCount > 0 && compiler->locals[compiler->localCount - 1].depth > compiler->scopeDepth) {
        if (compiler->locals[compiler->localCount - 1].isCaptured) {
            emitByte(compiler, OP_CLOSE_UPVALUE);
        } else {
            emitByte(compiler, OP_POP);
        }

        compiler->localCount--;
    }
}

static void grouping(Compiler *compiler, bool canAssign) {
    expression(compiler);
    eat(compiler->parser, TK_RPAREN, "Expect ')' after expression.");
}

static bool isOct(Token tk) {
    return tk.start[0] == '0' && (tk.start[1] == 'o' || tk.start[1] == 'O' || tk.start[1] == 'q' || tk.start[1] == 'Q');
}

static Value parseNumber(Compiler *compiler) {
    char *buf = ALLOCATE(compiler->parser->vm, char, compiler->parser->previous.len + 1);
    char *ptr = buf;
    
    for (int i = 0; i < compiler->parser->previous.len; ++i) {
        char c = compiler->parser->previous.start[i];
        
        if (c != '_') {
            *(ptr++) = c;
        }
    }
    
    *ptr = '\0';
    double num;
    if (isOct(compiler->parser->previous)) {
        num = strtol(buf + 2, NULL, 8); // + 2 to skip the octal indicator.
    } else {
        num = strtod(buf, NULL);
    }
    FREE_ARRAY(compiler->parser->vm, char, buf, compiler->parser->previous.len + 1);
    
    return NUMBER_VAL(num);
}

static void number(Compiler *compiler, bool canAssign) {
    emitConstant(compiler, parseNumber(compiler));
}

static void and_(Compiler *compiler, bool canAssign) {
    int endJump = emitJump(compiler, OP_JUMP_IF_FALSE);

    emitByte(compiler, OP_POP);
    parsePrecedence(compiler, PREC_AND);

    patchJump(compiler, endJump);
}

static void or_(Compiler *compiler, bool canAssign) {
    int endJump = emitJump(compiler, OP_JUMP_IF_TRUE);

    emitByte(compiler, OP_POP);

    parsePrecedence(compiler, PREC_OR);
    patchJump(compiler, endJump);
}

static void orr(Compiler *compiler, bool canAssgin) {
    // This means it is being used like x = val || otherVal
    IlexTokenType operatorType = compiler->parser->previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence(compiler, (Precedence)(rule->precedence + 1));
    
    emitByte(compiler, OP_OR);
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

static Value parseString(Compiler *compiler) {
    Parser *parser = compiler->parser;
    int strLen = parser->previous.len - 2;
    char *str = ALLOCATE(parser->vm, char, strLen + 1);
    
    memcpy(str, parser->previous.start + 1, strLen);
    int len = parseEscapeSequences(str, strLen);
    
    if (len != strLen) {
        str = SHRINK_ARRAY(parser->vm, str, char, strLen + 1, len + 1);
    }
    str[len] = '\0';
    
    return OBJ_VAL(takeString(parser->vm, str, len));
}

static void string(Compiler *compiler, bool canAssign) {
    emitConstant(compiler, parseString(compiler));
}

static void array(Compiler *compiler, bool canAssign) {
    int count = 0;
    
    do {
        if (check(compiler, TK_RBRACKET)) {
            break;
        }
        
        expression(compiler);
        if (++count > 255) {
            error(compiler->parser, "Can't have more than 255 items in an array initializer.");
        }
    } while (match(compiler, TK_COMMA));
    
    emitBytes(compiler, OP_NEW_ARRAY, count);
    eat(compiler->parser, TK_RBRACKET, "Expect ']' after array elements.");
}

static void map(Compiler *compiler, bool canAssign) {
    int count = 0;
    
    do {
        if (check(compiler, TK_RBRACE)) {
            break;
        }
    
        expression(compiler);
        eat(compiler->parser, TK_COLON, "Expect ':'.");
        expression(compiler);
        ++count;
    } while (match(compiler, TK_COMMA));
    
    emitBytes(compiler, OP_NEW_MAP, count);
    eat(compiler->parser, TK_RBRACE, "Expect closing '}'.");
}

static void set(Compiler *compiler, bool canAssign) {
    int count = 0;
    do {
        if (check(compiler, TK_GR)) {
            break;
        }
    
        expression(compiler);
        ++count;
    } while (match(compiler, TK_COMMA));
    
    emitBytes(compiler, OP_NEW_SET, count);
    eat(compiler->parser, TK_GR, "Expect closing '>'.");
}

static void index_(Compiler *compiler, bool canAssign) {
    if (match(compiler, TK_COLON)) {
        emitByte(compiler, OP_EMPTY);
        expression(compiler);
        emitByte(compiler, OP_SLICE);
        eat(compiler->parser, TK_RBRACKET, "Expect closing ']'.");
        
        return;
    }
    
    expression(compiler);
    
    if (match(compiler, TK_COLON)) {
        if (check(compiler, TK_RBRACKET)) {
            emitByte(compiler, OP_EMPTY);
        } else {
            expression(compiler);
        }
        emitByte(compiler, OP_SLICE);
        eat(compiler->parser, TK_RBRACKET, "Expect closing ']'.");
        
        return;
    }
    
    eat(compiler->parser, TK_RBRACKET, "Expect closing ']'.");
    
    if (canAssign && match(compiler, TK_ASSIGN)) {
        expression(compiler);
        emitByte(compiler, OP_INDEX_ASSIGN);
    } else if (canAssign && match(compiler, TK_PLUSEQ)) {
        expression(compiler);
        emitBytes(compiler, OP_INDEX_PUSH, OP_ADD);
        emitByte(compiler, OP_INDEX_ASSIGN);
    } else if (canAssign && match(compiler, TK_MINUSEQ)) {
        expression(compiler);
        emitBytes(compiler, OP_INDEX_PUSH, OP_SUB);
        emitByte(compiler, OP_INDEX_ASSIGN);
    } else if (canAssign && match(compiler, TK_MULEQ)) {
        expression(compiler);
        emitBytes(compiler, OP_INDEX_PUSH, OP_MUL);
        emitByte(compiler, OP_INDEX_ASSIGN);
    } else if (canAssign && match(compiler, TK_DIVEQ)) {
        expression(compiler);
        emitBytes(compiler, OP_INDEX_PUSH, OP_DIV);
        emitByte(compiler, OP_INDEX_ASSIGN);
    } else if (canAssign && match(compiler, TK_POWEQ)) {
        expression(compiler);
        emitBytes(compiler, OP_INDEX_PUSH, OP_POW);
        emitByte(compiler, OP_INDEX_ASSIGN);
    } else if (canAssign && match(compiler, TK_MODEQ)) {
        expression(compiler);
        emitBytes(compiler, OP_INDEX_PUSH, OP_MOD);
        emitByte(compiler, OP_INDEX_ASSIGN);
    } else if (canAssign && match(compiler, TK_NULL_COALESCE_EQ)) {
        expression(compiler);
        emitBytes(compiler, OP_INDEX_PUSH, OP_NULL_COALESCE);
        emitByte(compiler, OP_INDEX_ASSIGN);
    } else if (canAssign && match(compiler, TK_INC)) {
        emitBytes(compiler, OP_INDEX_PUSH, OP_INC);
        emitByte(compiler, OP_INDEX_ASSIGN);
    } else if (canAssign && match(compiler, TK_DEC)) {
        emitBytes(compiler, OP_INDEX_PUSH, OP_DEC);
        emitByte(compiler, OP_INDEX_ASSIGN);
    } else {
        emitByte(compiler, OP_INDEX);
    }
}

static void checkIfConst(Compiler *compiler, uint8_t setOp, int arg) {
    if (setOp == OP_SET_LOCAL) {
        if (compiler->locals[arg].isConst) {
            // TODO(Skyler): Find a better way to do this.
            char *name = (char*)malloc(compiler->locals[arg].name.len + 1);
            memcpy(name, compiler->locals[arg].name.start, compiler->locals[arg].name.len);
            name[compiler->locals[arg].name.len] = '\0';
            error(compiler->parser, "Cannot assign to const variable '%s'.", name);
            free(name);
        }
    } else if (setOp == OP_SET_GLOBAL) {
        Value _;
        ObjString *name = AS_STRING(currentChunk(compiler)->constants.values[arg]);
        if (tableGet(&compiler->parser->vm->consts, name, &_)) {
            error(compiler->parser, "Cannot assign to const variable '%s'.", name->str);
        }
    } else if (setOp == OP_SET_SCRIPT) {
        Value _;
        ObjString *name = AS_STRING(currentChunk(compiler)->constants.values[arg]);
        if (tableGet(&compiler->parser->vm->consts, name, &_)) {
            error(compiler->parser, "Cannot assign to const variable '%s'.", name->str);
        }
    }
}

static void namedVariable(Compiler *compiler, Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(compiler, &name, false);

    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(compiler, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = (int)identifierConstant(compiler, &name);
        ObjString *str = copyString(compiler->parser->vm, name.start, name.len);
        Value value;
        if (tableGet(&compiler->parser->vm->globals, str, &value)) {
            getOp = OP_GET_GLOBAL;
            canAssign = false;
        } else {
            getOp = OP_GET_SCRIPT;
            setOp = OP_SET_SCRIPT;
        }
    }

#define EMIT_OP_EQ(token) do { \
                              checkIfConst(compiler, setOp, arg); \
                              namedVariable(compiler, name, false); \
                              expression(compiler); \
                              emitByte(compiler, token); \
                              emitByteShort(compiler, setOp, (uint16_t)arg); \
                          } while (false) \
    
    if (canAssign && match(compiler, TK_ASSIGN)) {
        checkIfConst(compiler, setOp, arg);
        expression(compiler);
        emitByteShort(compiler, setOp, (uint16_t)arg);
    } else if (canAssign && match(compiler, TK_PLUSEQ)) {
        EMIT_OP_EQ(OP_ADD);
    } else if (canAssign && match(compiler, TK_MINUSEQ)) {
        EMIT_OP_EQ(OP_SUB);
    } else if (canAssign && match(compiler, TK_MULEQ)) {
        EMIT_OP_EQ(OP_MUL);
    } else if (canAssign && match(compiler, TK_DIVEQ)) {
        EMIT_OP_EQ(OP_DIV);
    } else if (canAssign && match(compiler, TK_POWEQ)) {
        EMIT_OP_EQ(OP_POW);
    } else if (canAssign && match(compiler, TK_MODEQ)) {
        EMIT_OP_EQ(OP_MOD);
    } else if (canAssign && match(compiler, TK_BIT_ANDEQ)) {
        EMIT_OP_EQ(OP_BIT_AND);
    } else if (canAssign && match(compiler, TK_BIT_OREQ)) {
        EMIT_OP_EQ(OP_BIT_OR);
    } else if (canAssign && match(compiler, TK_BIT_XOREQ)) {
        EMIT_OP_EQ(OP_BIT_XOR);
    } else if (canAssign && match(compiler, TK_NULL_COALESCE_EQ)) {
        EMIT_OP_EQ(OP_NULL_COALESCE);
    } else if (canAssign && match(compiler, TK_INC)) {
        checkIfConst(compiler, setOp, arg);
        namedVariable(compiler, name, false);
        emitByte(compiler, OP_INC);
        emitByteShort(compiler, setOp, (uint16_t)arg);
    } else if (canAssign && match(compiler, TK_DEC)) {
        checkIfConst(compiler, setOp, arg);
        namedVariable(compiler, name, false);
        emitByte(compiler, OP_DEC);
        emitByteShort(compiler, setOp, (uint16_t)arg);
    } else {
        emitByteShort(compiler, getOp, (uint16_t)arg);
    }

#undef EMIT_OP_EQ
}

static void variable(Compiler *compiler, bool canAssign) {
    namedVariable(compiler, compiler->parser->previous, canAssign);
}

static Token syntheticToken(const char *str) {
    Token token;
    token.start = str;
    token.len = (int)strlen(str);

    return token;
}

static void super_(Compiler *compiler, bool canAssign) {
    if (compiler->class == NULL) {
        error(compiler->parser, "Can't use 'super' outside of a class.");
    } else if (!compiler->class->hasSuperclass) {
        error(compiler->parser, "Can't use 'super' in a class woth no superclass.");
    }

    eat(compiler->parser, TK_DOT, "Expect '.' after 'super'.");
    eat(compiler->parser, TK_IDENT, "Expect superclass method name.");
    uint16_t name = identifierConstant(compiler, &compiler->parser->previous);

    namedVariable(compiler, syntheticToken("this"), false);
    if (match(compiler, TK_LPAREN)) {
        uint8_t argc = argumentList(compiler);
        namedVariable(compiler, syntheticToken("super"), false);
        emitByteShort(compiler, OP_SUPER_INVOKE, name);
        emitByte(compiler, argc);
    } else {
        namedVariable(compiler, syntheticToken("super"), false);
        emitByteShort(compiler, OP_GET_SUPER, name);
    }
}

static void this_(Compiler *compiler, bool canAssign) {
    if (compiler->class == NULL) {
        error(compiler->parser, "Can't use 'this' outside of a class.");
        return;
    }

    variable(compiler, false);
}

static void binary(Compiler *compiler, bool canAssign) {
    IlexTokenType operatorType = compiler->parser->previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence(compiler, (Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TK_NOTEQ:         emitByte(compiler, OP_NOTEQ); break;
        case TK_EQ:            emitByte(compiler, OP_EQ); break;
        case TK_GR:            emitByte(compiler, OP_GR); break;
        case TK_GREQ:          emitByte(compiler, OP_GREQ); break;
        case TK_LT:            emitByte(compiler, OP_LT); break;
        case TK_LTEQ:          emitByte(compiler, OP_LTEQ); break;
        case TK_PLUS:          emitByte(compiler, OP_ADD); break;
        case TK_MINUS:         emitByte(compiler, OP_SUB); break;
        case TK_MUL:           emitByte(compiler, OP_MUL); break;
        case TK_DIV:           emitByte(compiler, OP_DIV); break;
        case TK_POW:           emitByte(compiler, OP_POW); break;
        case TK_MOD:           emitByte(compiler, OP_MOD); break;
        case TK_BIT_AND:       emitByte(compiler, OP_BIT_AND); break;
        case TK_BIT_OR:        emitByte(compiler, OP_BIT_OR); break;
        case TK_BIT_XOR:       emitByte(compiler, OP_BIT_XOR); break;
        case TK_BIT_LS:        emitByte(compiler, OP_BIT_LS); break;
        case TK_BIT_RS:        emitByte(compiler, OP_BIT_RS); break;
        case TK_NULL_COALESCE: emitByte(compiler, OP_NULL_COALESCE); break;
        default: return; // Unreachable.
    }
}

static void ternary(Compiler *compiler, bool canAssign) {
    int elseJump = emitJump(compiler, OP_JUMP_IF_FALSE);
    
    emitByte(compiler, OP_POP);
    expression(compiler);
    
    int endJump = emitJump(compiler, OP_JUMP);
    
    patchJump(compiler, elseJump);
    emitByte(compiler, OP_POP); // Condition.
    
    eat(compiler->parser, TK_COLON, "Expect ':' after expression.");
    expression(compiler);
    
    patchJump(compiler, endJump);
}

static void call(Compiler *compiler, bool canAssign) {
    uint8_t argCount = argumentList(compiler);
    emitBytes(compiler, OP_CALL, argCount);
}

static void dot(Compiler *compiler, bool canAssign) {
    eat(compiler->parser, TK_IDENT, "Expect property name after '.'.");
    uint16_t name = identifierConstant(compiler, &compiler->parser->previous);

#define EMIT_OP_EQ(token) do { \
                              emitByteShort(compiler, OP_GET_PROPERTY_NO_POP, name); \
                              expression(compiler); \
                              emitByte(compiler, token); \
                              emitByteShort(compiler, OP_SET_PROPERTY, name); \
                          } while (false) \
    
    if (canAssign && match(compiler, TK_ASSIGN)) {
        expression(compiler);
        emitByteShort(compiler, OP_SET_PROPERTY, name);
    } else if (canAssign && match(compiler, TK_PLUSEQ)) {
        EMIT_OP_EQ(OP_ADD);
    }  else if (canAssign && match(compiler, TK_MINUSEQ)) {
        EMIT_OP_EQ(OP_SUB);
    }  else if (canAssign && match(compiler, TK_MULEQ)) {
        EMIT_OP_EQ(OP_MUL);
    } else if (canAssign && match(compiler, TK_DIVEQ)) {
        EMIT_OP_EQ(OP_DIV);
    } else if (canAssign && match(compiler, TK_POWEQ)) {
        EMIT_OP_EQ(OP_POW);
    } else if (canAssign && match(compiler, TK_MODEQ)) {
        EMIT_OP_EQ(OP_MOD);
    } else if (canAssign && match(compiler, TK_BIT_ANDEQ)) {
        EMIT_OP_EQ(OP_BIT_AND);
    } else if (canAssign && match(compiler, TK_BIT_OREQ)) {
        EMIT_OP_EQ(OP_BIT_OR);
    } else if (canAssign && match(compiler, TK_BIT_XOREQ)) {
        EMIT_OP_EQ(OP_BIT_XOR);
    } else if (canAssign && match(compiler, TK_NULL_COALESCE_EQ)) {
        EMIT_OP_EQ(OP_NULL_COALESCE);
    } else if (canAssign && match(compiler, TK_INC)) {
        emitByteShort(compiler, OP_GET_PROPERTY_NO_POP, name);
        emitByte(compiler, OP_INC);
        emitByteShort(compiler, OP_SET_PROPERTY, name);
    } else if (canAssign && match(compiler, TK_DEC)) {
        emitByteShort(compiler, OP_GET_PROPERTY_NO_POP, name);
        emitByte(compiler, OP_DEC);
        emitByteShort(compiler, OP_SET_PROPERTY, name);
    } else if (match(compiler, TK_LPAREN)) {
            uint8_t argc = argumentList(compiler);
            emitByteShort(compiler, OP_INVOKE, name);
            emitByte(compiler, argc);
    } else {
        emitByteShort(compiler, OP_GET_PROPERTY, name);
    }

#undef EMIT_OP_EQ
}

static void scope(Compiler *compiler, bool canAssign) {
    eat(compiler->parser, TK_IDENT, "Expect property name after '::'.");
    uint16_t name = identifierConstant(compiler, &compiler->parser->previous);

    if (match(compiler, TK_LPAREN)) {
        int argc = argumentList(compiler);
        emitByteShort(compiler, OP_INVOKE, name);
        emitByte(compiler, argc);
    } else {
        emitByteShort(compiler, OP_GET_PROPERTY, name);
    }
}

static void literal(Compiler *compiler, bool canAssign) {
    switch (compiler->parser->previous.type) {
        case TK_FALSE: emitByte(compiler, OP_FALSE); break;
        case TK_NULL:  emitByte(compiler, OP_NULL);  break;
        case TK_TRUE:  emitByte(compiler, OP_TRUE);  break;
        default: return; // Unreachable.
    }
}

static void unary(Compiler *compiler, bool canAssign) {
    IlexTokenType op = compiler->parser->previous.type;

    parsePrecedence(compiler, PREC_UNARY);

    switch (op) {
        case TK_NOT:     emitByte(compiler, OP_NOT);     break;
        case TK_BIT_NOT: emitByte(compiler, OP_BIT_NOT); break;
        case TK_MINUS:   emitByte(compiler, OP_NEG);     break;
        default: return;
    }
}

static void inc(Compiler *compiler, bool canAssign) {
    emitByte(compiler, OP_INC);
}

static void dec(Compiler *compiler, bool canAssign) {
    emitByte(compiler, OP_DEC);
}

static void anon(Compiler *compiler, bool canAssign) {
    Compiler functionCompiler;
    initCompiler(compiler->parser, &functionCompiler, compiler, TYPE_ANON);
    beginScope(&functionCompiler);
    
    if (match(&functionCompiler, TK_ARROW)) {
        if (match(&functionCompiler, TK_LBRACE)) {
            block(&functionCompiler);
        } else {
            expression(&functionCompiler);
            emitByte(&functionCompiler, OP_RETURN);
        }
    } else if (match(&functionCompiler, TK_BIT_OR)) {
        if (!check(&functionCompiler, TK_BIT_OR)) {
            do {
                functionCompiler.function->arity++;
                if (functionCompiler.function->arity > 255) {
                    errorAtCurrent(functionCompiler.parser, "Can't have more than 255 parameters.");
                }
            
                uint16_t constant = parseVariable(&functionCompiler, "Expect parameter name.");
                defineVariable(&functionCompiler, constant, false); // TODO: Should this be true?
            } while (match(&functionCompiler, TK_COMMA));
        }
        eat(functionCompiler.parser, TK_BIT_OR, "Expect '|' after parameters.");
        
        if (match(&functionCompiler, TK_ARROW)) {
            eat(functionCompiler.parser, TK_LBRACE, "Expect '{' after '->'.");
            block(&functionCompiler);
        } else {
            expression(&functionCompiler);
            emitByte(&functionCompiler, OP_RETURN);
        }
    }
    
    ObjFunction *function = endCompiler(&functionCompiler);
    emitByteShort(compiler, OP_CLOSURE, makeConstant(compiler, OBJ_VAL(function)));
    
    for (int i = 0; i < function->upvalueCount; ++i) {
        emitByte(compiler, functionCompiler.upvalues[i].isLocal ? 1 : 0);
        emitShort(compiler, functionCompiler.upvalues[i].index);
    }
}

//                              prefix, infix, precedence
ParseRule rules[] = {
        [TK_LPAREN]           = {grouping, call,    PREC_CALL},
        [TK_RPAREN]           = {NULL,     NULL,    PREC_NONE},
        [TK_LBRACE]           = {map,      NULL,    PREC_NONE},
        [TK_RBRACE]           = {NULL,     NULL,    PREC_NONE},
        [TK_LBRACKET]         = {array,    index_,  PREC_CALL},
        [TK_RBRACKET]         = {NULL,     NULL,    PREC_NONE},
        [TK_COMMA]            = {NULL,     NULL,    PREC_NONE},
        [TK_DOT]              = {NULL,     dot,     PREC_CALL},
        [TK_SCOPE]            = {NULL,     scope,   PREC_CALL},
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
        [TK_BIT_NOT]          = {unary,    NULL,    PREC_NONE},
        [TK_NOTEQ]            = {NULL,     binary,  PREC_EQUALITY},
        [TK_ASSIGN]           = {NULL,     NULL,    PREC_NONE},
        [TK_EQ]               = {NULL,     binary,  PREC_EQUALITY},
        [TK_GR]               = {NULL,     binary,  PREC_COMPARISON},
        [TK_GREQ]             = {NULL,     binary,  PREC_COMPARISON},
        [TK_LT]               = {set,      binary,  PREC_COMPARISON},
        [TK_LTEQ]             = {NULL,     binary,  PREC_COMPARISON},
        [TK_BIT_AND]          = {NULL,     binary,  PREC_BIT_AND},
        [TK_BIT_OR]           = {NULL,     binary,  PREC_BIT_OR},
        [TK_BIT_XOR]          = {NULL,     binary,  PREC_BIT_XOR},
        [TK_BIT_LS]           = {NULL,     binary,  PREC_BIT_SHIFT},
        [TK_BIT_RS]           = {NULL,     binary,  PREC_BIT_SHIFT},
        [TK_IDENT]            = {variable, NULL,    PREC_NONE},
        [TK_STRING]           = {string,   NULL,    PREC_NONE},
        [TK_NUMBER]           = {number,   NULL,    PREC_NONE},
        [TK_AND]              = {NULL,     and_,    PREC_AND},
        [TK_CLASS]            = {NULL,     NULL,    PREC_NONE},
        [TK_ELSE]             = {NULL,     NULL,    PREC_NONE},
        [TK_FALSE]            = {literal,  NULL,    PREC_NONE},
        [TK_FOR]              = {NULL,     NULL,    PREC_NONE},
        [TK_FN]               = {anon,     NULL,    PREC_NONE},
        [TK_ARROW]            = {NULL,     NULL,    PREC_NONE},
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
        [TK_CONST_DECL]       = {NULL,     NULL,    PREC_NONE},
        [TK_ENUM]             = {NULL,     NULL,    PREC_NONE},
        [TK_WHILE]            = {NULL,     NULL,    PREC_NONE},
        [TK_DO]               = {NULL,     NULL,    PREC_NONE},
        [TK_SWITCH]           = {NULL,     NULL,    PREC_NONE},
        [TK_CASE]             = {NULL,     NULL,    PREC_NONE},
        [TK_DEFAULT]          = {NULL,     NULL,    PREC_NONE},
        [TK_ASSERT]           = {NULL,     NULL,    PREC_NONE},
        [TK_PANIC]            = {NULL,     NULL,    PREC_NONE},
        [TK_INC]              = {NULL,     inc,     PREC_TERM},
        [TK_DEC]              = {NULL,     dec,     PREC_TERM},
        [TK_TER]              = {NULL,     ternary, PREC_ASSIGN},
        [TK_USE]              = {NULL,     NULL,    PREC_NONE},
        [TK_FROM]             = {NULL,     NULL,    PREC_NONE},
        [TK_AS]               = {NULL,     NULL,    PREC_NONE},
        [TK_BREAK]            = {NULL,     NULL,    PREC_NONE},
        [TK_CONTINUE]         = {NULL,     NULL,    PREC_NONE},
        [TK_ERROR]            = {NULL,     NULL,    PREC_NONE},
        [TK_EOF]              = {NULL,     NULL,    PREC_NONE},
};

static void parsePrecedence(Compiler *compiler, Precedence precedence) {
    Parser *parser = compiler->parser;
    advance(parser);

    ParseFn prefixRule = getRule(parser->previous.type)->prefix;
    
    if (prefixRule == NULL) {
        error(parser, "Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGN;
    prefixRule(compiler, canAssign);

    while (precedence <= getRule(parser->current.type)->precedence) {
        bool setInfixToOrr = false;
        if (parser->current.type == TK_OR && precedence == PREC_ASSIGN) {
            setInfixToOrr = true;
        }
        
        advance(parser);
        
        ParseFn infixRule = getRule(parser->previous.type)->infix;
        if (setInfixToOrr) {
            infixRule = orr;
        }
        
        infixRule(compiler, canAssign);
    }

    if (canAssign && match(compiler, TK_ASSIGN)) {
        error(parser, "Invalid assignment target.");
    }
}

static void synchronize(Parser *parser) {
    parser->panicMode = false;

    while (parser->current.type != TK_EOF) {
        if (parser->previous.type == TK_SEMICOLON) {
            return;
        }

        switch (parser->current.type) {
            case TK_CLASS:
            case TK_FN:
            case TK_VAR:
            case TK_CONST:
            case TK_FOR:
            case TK_IF:
            case TK_WHILE:
            case TK_DO:
            case TK_RETURN:
            case TK_ASSERT:
            case TK_SWITCH:
            case TK_USE:
            case TK_FROM:
            case TK_AS:
            case TK_BREAK:
                return;

            default:
                ; // Do nothing.
        }

        advance(parser);
        return;
    }
}

static ParseRule* getRule(IlexTokenType type) {
    return &rules[type];
}

static void expression(Compiler *compiler) {
    parsePrecedence(compiler, PREC_ASSIGN);
}

static void block(Compiler *compiler) {
    while (!check(compiler, TK_RBRACE) && !check(compiler, TK_EOF)) {
        declaration(compiler);
    }

    eat(compiler->parser, TK_RBRACE, "Expect '}' after block.");
}

static void function(Compiler *compiler, FunctionType type) {
    Compiler functionCompiler;
    initCompiler(compiler->parser, &functionCompiler, compiler, type);
    beginScope(&functionCompiler);

    eat(functionCompiler.parser, TK_LPAREN, "Expect '(' after function name.");
    if (!check(&functionCompiler, TK_RPAREN)) {
        do {
            functionCompiler.function->arity++;
            if (functionCompiler.function->arity > 255) {
                errorAtCurrent(functionCompiler.parser, "Can't have more than 255 parameters.");
            }

            uint16_t constant = parseVariable(&functionCompiler, "Expect parameter name.");
            defineVariable(&functionCompiler, constant, false); // TODO: Should this be true?
        } while (match(&functionCompiler, TK_COMMA));
    }
    eat(functionCompiler.parser, TK_RPAREN, "Expect ')' after parameters.");
    eat(functionCompiler.parser, TK_LBRACE, "Expect '{' before function body.");
    block(&functionCompiler);

    ObjFunction *function = endCompiler(&functionCompiler);
    emitByteShort(compiler, OP_CLOSURE, makeConstant(compiler, OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; ++i) {
        emitByte(compiler, functionCompiler.upvalues[i].isLocal ? 1 : 0);
        emitShort(compiler, functionCompiler.upvalues[i].index);
    }
}

static void method(Compiler *compiler) {
    eat(compiler->parser, TK_IDENT, "Expect method name.");
    uint16_t constant = identifierConstant(compiler, &compiler->parser->previous);

    FunctionType type = TYPE_METHOD;
    if (compiler->parser->previous.len == 4 &&
        memcmp(compiler->parser->previous.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }
    function(compiler, type);
    emitByteShort(compiler, OP_METHOD, constant);
}

static void classDeclaration(Compiler *compiler) {
    eat(compiler->parser, TK_IDENT, "Expect class name.");
    Token className = compiler->parser->previous;
    uint16_t nameConstant = identifierConstant(compiler, &compiler->parser->previous);
    declareVariable(compiler);

    emitByteShort(compiler, OP_CLASS, nameConstant);
    defineVariable(compiler, nameConstant, false);

    ClassCompiler classCompiler;
    classCompiler.enclosing = compiler->class;
    classCompiler.hasSuperclass = false;
    compiler->class = &classCompiler;

    if (match(compiler, TK_LT)) {
        eat(compiler->parser, TK_IDENT, "Expect superclass name.");
        variable(compiler, false);

        if (identifiersEqual(&className, &compiler->parser->previous)) {
            error(compiler->parser, "A class can't inherit from itself.");
        }

        beginScope(compiler);
        addLocal(compiler, syntheticToken("super"));
        defineVariable(compiler, 0, false);

        namedVariable(compiler, className, false);
        emitByte(compiler, OP_INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(compiler, className, false);
    eat(compiler->parser, TK_LBRACE, "Expect '{' before class body.");
    while (!check(compiler, TK_RBRACE) && !check(compiler, TK_EOF)) {
        method(compiler);
    }
    eat(compiler->parser, TK_RBRACE, "Expect '}' after class body.");
    emitByte(compiler, OP_POP);

    if (classCompiler.hasSuperclass) {
        endScope(compiler);
    }

    compiler->class = compiler->class->enclosing;
}

static void fnDeclaration(Compiler *compiler) {
    uint16_t global = parseVariable(compiler, "Expect function name after 'fn'.");
    function(compiler, TYPE_FUNCTION);
    defineVariable(compiler, global, false);
}

static void varDeclaration(Compiler *compiler, bool isConst) {
    do {
        uint16_t global = parseVariable(compiler, "Expect variable name.");

        if (match(compiler, TK_ASSIGN) || isConst) {
            expression(compiler);
        } else {
            emitByte(compiler, OP_NULL);
        }

        defineVariable(compiler, global, isConst);
    } while (match(compiler, TK_COMMA));

    match(compiler, TK_SEMICOLON);
}

static void varDeclaration2(Compiler *compiler, bool isConst) {
    eat(compiler->parser, TK_IDENT, "Expect variable name.");
    declareVariable(compiler);
    uint16_t global;

    if (compiler->scopeDepth > 0) {
        global = 0;
    } else {
        global = identifierConstant(compiler, &compiler->parser->previous);
    }

    if (!isConst) {
        eat(compiler->parser, TK_VAR_DECL, "Expect := after variable name.");
    } else {
        eat(compiler->parser, TK_CONST_DECL, "Expect ::= after variable name.");
    }
    expression(compiler);
    defineVariable(compiler, global, isConst);

    match(compiler, TK_SEMICOLON);
}

static void enumDeclaration(Compiler *compiler) {
    eat(compiler->parser, TK_IDENT, "Expect enum name.");

    uint16_t nameConstant = identifierConstant(compiler, &compiler->parser->previous);
    declareVariable(compiler);
    emitByteShort(compiler, OP_ENUM, nameConstant);
    eat(compiler->parser, TK_LBRACE, "Expect '{' before enum body.");
    int index = 0;

    do {
        if (check(compiler, TK_RBRACE)) {
            break;
        }

        eat(compiler->parser, TK_IDENT, "Expect enum value identifier.");
        uint16_t name = identifierConstant(compiler, &compiler->parser->previous);

        if (match(compiler, TK_ASSIGN)) {
            expression(compiler);
        } else {
            emitConstant(compiler, NUMBER_VAL(index));
        }

        emitByteShort(compiler, OP_ENUM_SET_VALUE, name);
        index++;
    } while (match(compiler, TK_COMMA));

    eat(compiler->parser, TK_RBRACE, "Expect '}' after enum body.");
    defineVariable(compiler, nameConstant, false);
}

static void expressionStatement(Compiler *compiler) {
    expression(compiler);
    match(compiler, TK_SEMICOLON);
    emitByte(compiler, OP_POP);
}

static int getArgCount(const uint8_t *code, const ValueArray constants, int ip) {
    switch (code[ip]) {
        case OP_NULL:
        case OP_TRUE:
        case OP_FALSE:
        case OP_POP:
        case OP_EQ:
        case OP_GR:
        case OP_LT:
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_POW:
        case OP_MOD:
        case OP_NOT:
        case OP_NEG:
        case OP_CLOSE_UPVALUE:
        case OP_RETURN:
        case OP_BREAK:
        case OP_OPEN_FILE:
            return 0;

        case OP_CONSTANT:
        case OP_GET_LOCAL:
        case OP_SET_LOCAL:
        case OP_GET_GLOBAL:
        case OP_GET_UPVALUE:
        case OP_SET_UPVALUE:
        case OP_GET_PROPERTY:
        case OP_GET_PROPERTY_NO_POP:
        case OP_SET_PROPERTY:
        case OP_GET_SUPER:
        case OP_CALL:
        case OP_METHOD:
        case OP_USE:
        case OP_MULTI_CASE:
        case OP_GET_SCRIPT:
        case OP_SET_SCRIPT:
        case OP_DEFINE_SCRIPT:
        case OP_NEW_MAP:
            return 1;

        case OP_JUMP:
        case OP_CMP_JMP:
        case OP_JUMP_IF_FALSE:
        case OP_LOOP:
        case OP_INVOKE:
        case OP_CLASS:
        case OP_USE_BUILTIN:
            return 2;

        case OP_USE_BUILTIN_VAR: {
            int argCount = code[ip + 2];

            return 2 + argCount;
        }

        case OP_CLOSURE: {
            int constant = code[ip + 1];
            ObjFunction* loadedFn = AS_FUNCTION(constants.values[constant]);

            // There is one byte for the constant, then two for each upvalue.
            return 1 + (loadedFn->upvalueCount * 2);
        }
    }

    return 0;
}

static void endLoop(Compiler *compiler, bool isDo) {
    if (!isDo && compiler->loop->end != -1) {
        patchJump(compiler, compiler->loop->end);
        emitByte(compiler, OP_POP); // Condition.
    } else if (isDo && compiler->loop->body != -1) {
        int offset = compiler->loop->end;
        int jump = compiler->loop->end - compiler->loop->start + 2;
        currentChunk(compiler)->code[offset] = (jump >> 8) & 0xff;
        currentChunk(compiler)->code[offset + 1] = jump & 0xff;
        emitByte(compiler, OP_POP); // Condition.
    }

    int i = compiler->loop->body;
    while (i < compiler->function->chunk.count) {
        if (compiler->function->chunk.code[i] == OP_BREAK) {
            compiler->function->chunk.code[i] = OP_JUMP;
            patchJump(compiler, i + 1);
            i += 3;
        } else {
            i += 1 + getArgCount(compiler->function->chunk.code, compiler->function->chunk.constants, i);
        }
    }

    compiler->loop = compiler->loop->enclosing;
}

static void forStatement(Compiler *compiler) {
    beginScope(compiler);
    eat(compiler->parser, TK_LPAREN, "Expect '(' after 'for'.");

    bool initializer = true;
    if (match(compiler, TK_SEMICOLON)) {
        initializer = false;
    } else if (match(compiler, TK_VAR)) {
        varDeclaration(compiler, false);
    } else if (check(compiler, TK_IDENT) && lookahead(compiler, TK_VAR_DECL)) {
        varDeclaration2(compiler, false);
    } else {
        expressionStatement(compiler);
    }

    if (initializer) {
        if (compiler->parser->previous.type != TK_SEMICOLON) {
            errorAtCurrent(compiler->parser, "Expect ';' after loop initializer.");
        }
    }

    Loop loop;
    loop.start = currentChunk(compiler)->count;
    loop.scopeDepth = compiler->scopeDepth;
    loop.enclosing = compiler->loop;
    compiler->loop = &loop;
    compiler->loop->end = -1; // Exit condition.

    if (!match(compiler, TK_SEMICOLON)) {
        expression(compiler);
        eat(compiler->parser, TK_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        compiler->loop->end = emitJump(compiler, OP_JUMP_IF_FALSE);
        emitByte(compiler, OP_POP); // Condition.
    }

    if (!match(compiler, TK_RPAREN)) {
        int bodyJump = emitJump(compiler, OP_JUMP);
        int incrementStart = currentChunk(compiler)->count;
        expression(compiler);
        emitByte(compiler, OP_POP);
        eat(compiler->parser, TK_RPAREN, "Expect ')' after for clauses.");

        emitLoop(compiler, compiler->loop->start);
        compiler->loop->start = incrementStart;
        patchJump(compiler, bodyJump);
    }

    compiler->loop->body = compiler->function->chunk.count;
    eat(compiler->parser, TK_LBRACE, "Expect '{' after ')'.");
    beginScope(compiler);
    block(compiler);
    endScope(compiler);

    emitLoop(compiler, compiler->loop->start);
    endLoop(compiler, false);
    endScope(compiler);
}

static void assertStatement(Compiler *compiler) {
    eat(compiler->parser, TK_LPAREN, "Expect '(' after 'assert'.");

    int constant = addConstant(compiler->parser->vm, currentChunk(compiler), OBJ_VAL(copyString(compiler->parser->vm, "no msg", 6)));

    expression(compiler);

    if (match(compiler, TK_COMMA)) {
        eat(compiler->parser, TK_STRING, "Expect assert error string after ','.");
        constant = addConstant(compiler->parser->vm,  currentChunk(compiler), OBJ_VAL(copyString(compiler->parser->vm, compiler->parser->previous.start + 1, compiler->parser->previous.len - 2)));
    }
    eat(compiler->parser, TK_RPAREN, "Expect ')' after condition.");
    match(compiler, TK_SEMICOLON);

    emitByteShort(compiler, OP_ASSERT, (uint16_t)constant);
}

static void panicStatement(Compiler *compiler) {
    eat(compiler->parser, TK_LPAREN, "Expect '(' after 'panic!'.");
    eat(compiler->parser, TK_STRING, "Expect panic! error string after '('.");
    int constant = addConstant(compiler->parser->vm,  currentChunk(compiler), OBJ_VAL(copyString(compiler->parser->vm, compiler->parser->previous.start + 1, compiler->parser->previous.len - 2)));
    eat(compiler->parser, TK_RPAREN, "Expect ')' after condition.");
    match(compiler, TK_SEMICOLON);

    emitByteShort(compiler, OP_PANIC, (uint16_t)constant);
}

static void switchStatement(Compiler *compiler) {
    int caseEnds[256];
    int caseCount = 0;

    eat(compiler->parser, TK_LPAREN, "Expect '(' after 'switch'.");
    expression(compiler);
    eat(compiler->parser, TK_RPAREN, "Expect ')' after expression.");
    eat(compiler->parser, TK_LBRACE, "Expect '{' after ')'.");
    eat(compiler->parser, TK_CASE, "Expect at least one 'case' block.");

    int nextJmp = -1;
    do {
        expression(compiler);
        int multipleCases = 0;
        if (match(compiler, TK_COMMA)) {
            do {
                multipleCases++;
                expression(compiler);
            } while (match(compiler, TK_COMMA));
            emitBytes(compiler, OP_MULTI_CASE, multipleCases);
        }
        if (!check(compiler, TK_COLON) && !check(compiler, TK_ARROW)) {
            eat(compiler->parser, TK_COLON, "Expect ':' or '->' after expression.");
        }

        uint8_t jmpType = check(compiler, TK_ARROW) ? OP_CMP_JMP_FALL : OP_CMP_JMP;
        int compareJump = emitJump(compiler, jmpType);

        bool dontJump = false;
        if (match(compiler, TK_ARROW)) {
            dontJump = true;
        } else {
            match(compiler, TK_COLON);
            nextJmp = -1;
        }

        statement(compiler);

        if (dontJump) {
            caseEnds[caseCount++] = -1;
        } else {
            caseEnds[caseCount++] = emitJump(compiler, OP_JUMP);
        }
        patchJump(compiler, compareJump);

        if (caseCount > 255) {
            errorAtCurrent(compiler->parser, "Switch statements can't have more than 255 case blocks");
        }

        if (!check(compiler, TK_CASE) && !check(compiler, TK_RBRACE) && !check(compiler, TK_DEFAULT)) {
            char *msg = newCStringLen(compiler->parser->current.start, compiler->parser->current.len);
            error(compiler->parser, "Unexpected token '%s'.", msg);
            free(msg);
            synchronize(compiler->parser);
        }

    } while (match(compiler, TK_CASE));

    if (match(compiler,TK_DEFAULT)){
        emitByte(compiler, OP_POP); // expression.
        eat(compiler->parser, TK_COLON, "Expect ':' after 'default'."); // -> would not make sense here.
        statement(compiler);
    }

    if (match(compiler,TK_CASE)){
        error(compiler->parser, "Unexpected case after 'default'.");
    }

    eat(compiler->parser, TK_RBRACE, "Expect '}' after cases.");

    for (int i = 0; i < caseCount; i++) {
        if (caseEnds[i] >= 0) {
            patchJump(compiler, caseEnds[i]);
        }
    }
}

static void ifStatement(Compiler *compiler) {
    eat(compiler->parser, TK_LPAREN, "Expect '(' after 'if'.");
    expression(compiler);
    eat(compiler->parser, TK_RPAREN, "Expect ')' after condition.");

    int thenJump = emitJump(compiler, OP_JUMP_IF_FALSE);
    emitByte(compiler, OP_POP);

    eat(compiler->parser, TK_LBRACE, "Expect '{' after ')'.");
    beginScope(compiler);
    block(compiler);
    endScope(compiler);

    int elseJump = emitJump(compiler, OP_JUMP);
    patchJump(compiler, thenJump);
    emitByte(compiler, OP_POP);

    if (match(compiler, TK_ELIF)) {
        ifStatement(compiler);
    } else if (match(compiler, TK_ELSE)) {
        if (match(compiler, TK_IF)) {
            ifStatement(compiler);
        } else {
            eat(compiler->parser, TK_LBRACE, "Expect '{' after 'else'.");
            beginScope(compiler);
            block(compiler);
            endScope(compiler);
        }
    }
    patchJump(compiler, elseJump);
}

static void whileStatement(Compiler *compiler) {
    Loop loop;
    loop.start = currentChunk(compiler)->count;
    loop.scopeDepth = compiler->scopeDepth;
    loop.enclosing = compiler->loop;
    compiler->loop = &loop;

    eat(compiler->parser, TK_LPAREN, "Expect '(' after 'while'.");
    expression(compiler);
    eat(compiler->parser, TK_RPAREN, "Expect ')' after condition.");

    compiler->loop->end = emitJump(compiler, OP_JUMP_IF_FALSE);
    emitByte(compiler, OP_POP);
    compiler->loop->body = compiler->function->chunk.count;

    eat(compiler->parser, TK_LBRACE, "Expect '{' after ')'.");
    beginScope(compiler);
    block(compiler);
    endScope(compiler);

    emitLoop(compiler, compiler->loop->start);
    endLoop(compiler, false);
}

static void doWhileStatement(Compiler *compiler) {
    Loop loop;
    loop.start = currentChunk(compiler)->count;
    loop.scopeDepth = compiler->scopeDepth;
    loop.enclosing = compiler->loop;
    compiler->loop = &loop;

    compiler->loop->body = compiler->function->chunk.count;

    eat(compiler->parser, TK_LBRACE, "Expect '{' after 'do'.");
    beginScope(compiler);
    block(compiler);
    endScope(compiler);

    eat(compiler->parser, TK_WHILE, "Expect 'while' after '}'.");
    eat(compiler->parser, TK_LPAREN, "Expect '(' after 'while'.");
    expression(compiler);
    eat(compiler->parser, TK_RPAREN, "Expect ')' after condition.");

    compiler->loop->end = emitJump(compiler, OP_JUMP_DO_WHILE);
    endLoop(compiler, true);
}

static void useStatement(Compiler *compiler, bool isFrom) {
    if (compiler->scopeDepth > 0) {
        error(compiler->parser, "'use' must be at top level code.");
        return;
    }

    if (match(compiler, TK_LT)) {
        eat(compiler->parser, TK_IDENT, "Expect library name after '<'.");

        int idx = findBuiltInLib((char*)compiler->parser->previous.start, compiler->parser->previous.len);
        if (idx == -1) {
            error(compiler->parser, "Unknown library.");
            return;
        }

        compiler->currentScript = AS_SCRIPT(useBuiltInLib(compiler->parser->vm, idx));

        bool isAs = false;
        if (match(compiler, TK_AS)) {
            if (isFrom) {
                error(compiler->parser, "Can't have 'as' in a use from statement.");
                return;
            }

            eat(compiler->parser, TK_IDENT, "Expect name after 'as'.");
            compiler->currentLibName = identifierConstant(compiler, &compiler->parser->previous);
            isAs = true;
        }

        if (!isAs) {
            compiler->currentLibName = identifierConstant(compiler, &compiler->parser->previous);
        }
        declareVariable(compiler);

        emitBytes(compiler, OP_USE_BUILTIN, idx);
        emitShort(compiler, compiler->currentLibName);

        if (!isFrom) {
            defineVariable(compiler, compiler->currentLibName, false);
        }
        eat(compiler->parser, TK_GR, "Expect '>' after library name.");
    } else if (match(compiler, TK_STRING)) {
        uint16_t useConstant = makeConstant(compiler, OBJ_VAL(copyString(compiler->parser->vm,
                                                                         compiler->parser->previous.start + 1,
                                                                         compiler->parser->previous.len - 2)));
        emitByteShort(compiler, OP_USE, useConstant);
        emitByte(compiler, OP_POP);
        
        if (match(compiler, TK_AS)) {
            uint16_t useName = parseVariable(compiler, "Expect use name.");
            emitByte(compiler, OP_USE_VAR);
            defineVariable(compiler, useName, false); // TODO(Skyler): Should this be true?
        }
    } else if (match(compiler, TK_LBRACE)) {
        uint16_t variables[255];
        Token tokens[255];
        int varCount = 0;

        do {
            eat(compiler->parser, TK_IDENT, "Expect variable name.");
            tokens[varCount] = compiler->parser->previous;
            variables[varCount] = identifierConstant(compiler, &compiler->parser->previous);

            if (++varCount > 255) {
                error(compiler->parser, "Cannot 'use' more than 255 variables.");
            }
        } while(match(compiler, TK_COMMA));

        eat(compiler->parser, TK_RBRACE, "Expect '}' after variable list.");
        eat(compiler->parser, TK_FROM, "Expect 'from' after '}'");

        bool builtin = false;
        if (check(compiler, TK_LT)) {
            builtin = true;
        }

        useStatement(compiler, true);

        if (builtin) {
            emitByte(compiler, OP_POP);
            emitByte(compiler, OP_USE_BUILTIN_VAR);
            emitShortByte(compiler, compiler->currentLibName, varCount);

            for (int i = 0; i < varCount; ++i) {
                emitShort(compiler, variables[i]);
            }

            for (int i = varCount - 1; i >= 0; --i) {
                defineVariable(compiler, variables[i], false);
            }
        }
    } else if (match(compiler, TK_MUL)) {
        eat(compiler->parser, TK_FROM, "Expect 'from' after '*'.");

        bool builtin = false;
        if (check(compiler, TK_LT)) {
            builtin = true;
        }

        useStatement(compiler, true);

        if (compiler->currentScript == NULL) {
            error(compiler->parser, "Unknown scrip.");
            return;
        }

        Table *values = &compiler->currentScript->values;
        Entry *e;
        // Don't use values->count because we don't want vars starting with '$' or '_' and the number of those is not known.
        int varCount = 0;
        Token tokens[255];
        uint16_t variables[255];
        for (int i = 0; i < values->capacity; ++i) {
            e = &values->entries[i];

            // Vars starting with '$' are builtin and vars starting with '_' are private so skip them.
            if (e->key != NULL && e->key->str[0] != '$' && e->key->str[0] != '_') {
                Token fake;
                fake.start = e->key->str;
                fake.len = e->key->len;
                tokens[varCount++] = fake;
            }
        }

        for (int i = 0; i < varCount; ++i) {
            variables[i] = identifierConstant(compiler, &tokens[i]);
        }

        if (builtin) {
            emitByte(compiler, OP_POP);
            emitByte(compiler, OP_USE_BUILTIN_VAR);
            emitShortByte(compiler, compiler->currentLibName, varCount);

            for (int i = 0; i < varCount; ++i) {
                emitShort(compiler, variables[i]);
            }

            for (int i = varCount - 1; i >= 0; --i) {
                defineVariable(compiler, variables[i], false);
            }
        }
    }
    
    emitByte(compiler, OP_USE_END);
    match(compiler, TK_SEMICOLON);
}

static void continueStatement(Compiler *compiler) {
    if (compiler->loop == NULL) {
        error(compiler->parser, "Can't have 'continue' outside of a loop.");
        return;
    }

    match(compiler, TK_SEMICOLON);

    // Discard any locals created inside the loop.
    for (int i = compiler->localCount - 1; i >= 0 && compiler->locals[i].depth > compiler->loop->scopeDepth; --i) {
        emitByte(compiler, OP_POP);
    }

    emitLoop(compiler, compiler->loop->start);
}

static void breakStatement(Compiler *compiler) {
    if (compiler->loop == NULL) {
        error(compiler->parser, "Can't have 'break' outside of a loop.");
        return;
    }

    match(compiler, TK_SEMICOLON);

    // Discard any locals created inside the loop.
    for (int i = compiler->localCount - 1; i >= 0 && compiler->locals[i].depth > compiler->loop->scopeDepth; i--) {
        emitByte(compiler, OP_POP);
    }

    emitJump(compiler, OP_BREAK);
}

static void withStatement(Compiler *compiler) {
    if (compiler->isWithBlock) {
        error(compiler->parser, "Can't have a with statement inside a with statement.");
        return;
    }
    
    compiler->isWithBlock = true;
    eat(compiler->parser, TK_LPAREN, "Expect '(' after 'with'.");
    expression(compiler);
    eat(compiler->parser, TK_COMMA, "Expect ',' after path.");
    expression(compiler);
    eat(compiler->parser, TK_RPAREN, "Expect ')' after flags.");
    if (match(compiler, TK_AS)) {
        eat(compiler->parser, TK_IDENT, "Expect name after 'as'.");
        int len = compiler->parser->previous.len;
        compiler->withVarName = (char*)malloc(sizeof(char) * len + 1);
        // strcpy_s(compiler->withVarName, len, compiler->parser->previous.start);
        memcpy(compiler->withVarName, compiler->parser->previous.start, len);
        compiler->withVarName[len] = '\0';
    
        eat(compiler->parser, TK_LBRACE, "Expect '{' after name.");
    } else {
        eat(compiler->parser, TK_LBRACE, "Expect '{' after ')'.");
    }
    
    beginScope(compiler);
    
    int fileIdx = compiler->localCount;
    Local *local = &compiler->locals[compiler->localCount++];
    local->depth = compiler->scopeDepth;
    local->name = compiler->withVarName == NULL ? syntheticToken("file") : syntheticToken(compiler->withVarName);
    local->isCaptured = false;
    local->isConst = true;
    
    emitByte(compiler, OP_OPEN_FILE);
    block(compiler);
    emitByteShort(compiler, OP_CLOSE_FILE, fileIdx);
    endScope(compiler);
    
    compiler->isWithBlock = false;
    if (compiler->withVarName != NULL) {
        free(compiler->withVarName);
        compiler->withVarName = NULL;
    }
}

static void checkWithFile(Compiler *compiler) {
    if (compiler->isWithBlock) {
        Token token = compiler->withVarName == NULL ? syntheticToken("file") : syntheticToken(compiler->withVarName);
        int local = resolveLocal(compiler, &token, true);
        
        if (local != -1) {
            emitByteShort(compiler, OP_CLOSE_FILE, local);
            if (compiler->withVarName != NULL) {
                free(compiler->withVarName);
                compiler->withVarName = NULL;
            }
        }
    }
}

static void returnStatement(Compiler *compiler) {
    if (compiler->type == TYPE_SCRIPT) {
        error(compiler->parser, "Can't return from top-level code.");
    }
    
    if (match(compiler, TK_SEMICOLON)) {
        checkWithFile(compiler);
        emitReturn(compiler);
    } else {
        if (compiler->type == TYPE_INITIALIZER) {
            error(compiler->parser, "Can't return a value from an initializer.");
        }
        
        expression(compiler);
        match(compiler, TK_SEMICOLON);
        checkWithFile(compiler);
        emitByte(compiler, OP_RETURN);
    }
}

static void declaration(Compiler *compiler) {
    if (match(compiler, TK_CLASS)) {
        classDeclaration(compiler);
    } else if (match(compiler, TK_FN)) {
        fnDeclaration(compiler);
    } else if (match(compiler, TK_VAR)) {
        varDeclaration(compiler, false);
    } else if (check(compiler, TK_IDENT) && lookahead(compiler, TK_VAR_DECL)) {
        varDeclaration2(compiler, false);
    } else if (check(compiler, TK_IDENT) && lookahead(compiler, TK_CONST_DECL)) {
        varDeclaration2(compiler, true);
    } else if (match(compiler, TK_CONST)) {
        varDeclaration(compiler, true);
    } else if (match(compiler, TK_ENUM)) {
        enumDeclaration(compiler);
    } else {
        statement(compiler);
    }

    if (compiler->parser->panicMode) {
        synchronize(compiler->parser);
    }
}

static void statement(Compiler *compiler) {
    if (match(compiler, TK_FOR)) {
        forStatement(compiler);
    } else if (match(compiler, TK_IF)) {
        ifStatement(compiler);
    } else if (match(compiler, TK_RETURN)) {
        returnStatement(compiler);
    } else if (match(compiler, TK_WHILE)) {
        whileStatement(compiler);
    } else if (match(compiler, TK_DO)) {
        doWhileStatement(compiler);
    } else if (match(compiler, TK_ASSERT)) {
        assertStatement(compiler);
    } else if (match(compiler, TK_PANIC)) {
        panicStatement(compiler);
    } else if (match(compiler, TK_SWITCH)) {
        switchStatement(compiler);
    } else if (match(compiler, TK_USE)) {
        useStatement(compiler, false);
    } else if (match(compiler, TK_CONTINUE)) {
        continueStatement(compiler);
    } else if (match(compiler, TK_BREAK)) {
        breakStatement(compiler);
    } else if (match(compiler, TK_WITH)) {
        withStatement(compiler);
    } else if (match(compiler, TK_LBRACE)) {
        beginScope(compiler);
        block(compiler);
        endScope(compiler);
    } else {
        expressionStatement(compiler);
    }
}

ObjFunction *compile(VM *vm, ObjScript *script,  const char *source) {
    Parser parser;
    parser.vm = vm;
    parser.hadError = false;
    parser.panicMode = false;
    parser.script = script;

    initLexer(source);
    Compiler compiler;
    initCompiler(&parser, &compiler, NULL, TYPE_SCRIPT);
    firstAdvance(&compiler);

    advance(compiler.parser);
    while (!match(&compiler, TK_EOF)) {
        declaration(&compiler);
    }

    ObjFunction *function = endCompiler(&compiler);
    return parser.hadError ? NULL : function;
}

void markCompilerRoots(VM *vm) {
    Compiler *compiler = vm->compiler;

    while (compiler != NULL) {
        markObject(vm, (Obj*)compiler->function);
        compiler = compiler->enclosing;
    }
}
