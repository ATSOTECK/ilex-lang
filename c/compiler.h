//
// Created by Skyler on 3/12/22.
//

#ifndef __C_COMPILER_H__
#define __C_COMPILER_H__

#include "lexer.h"
#include "object.h"

typedef struct {
    VM *vm;
    Token current;
    Token previous;
    Token next;
    bool hadError;
    bool panicMode;
} Parser;

typedef struct {
    Token name;
    int depth;
    bool isConst;
    bool isCaptured;
} Local;

typedef struct {
    uint16_t index;
    bool isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT,
    TYPE_ANON,
} FunctionType;

typedef struct Loop {
    struct Loop *enclosing;
    int start;
    int body;
    int end;
    int scopeDepth;
} Loop;

typedef struct ClassCompiler {
    struct ClassCompiler *enclosing;
    bool hasSuperclass;
} ClassCompiler;

typedef struct Compiler {
    Parser *parser;
    struct Compiler *enclosing;
    ClassCompiler *class;
    ObjFunction *function;
    FunctionType type;

    Local *locals; //TODO(Skyler): Gow this.
    int localCount;
    Upvalue *upvalues; //TODO(Skyler): Gow this.
    int scopeDepth;
    uint16_t currentLibName;
    ObjScript *currentScript;
    Loop *loop;
    bool isWithBlock;
    char *withVarName;
} Compiler;

ObjFunction *compile(VM *vm, const char *source);
void markCompilerRoots(VM *vm);

#endif //__C_COMPILER_H__
