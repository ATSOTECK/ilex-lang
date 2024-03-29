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
    ObjScript *script;
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

typedef struct Loop {
    struct Loop *enclosing;
    int start;
    int body;
    int end;
    int scopeDepth;
} Loop;

typedef struct ClassCompiler {
    struct ClassCompiler *enclosing;
    Token name;
    bool hasSuperclass;
    bool staticMethod;
    bool abstractClass;
    Table privateVariables;
} ClassCompiler;

typedef struct Compiler {
    Parser *parser;
    Table stringConsts;
    struct Compiler *enclosing;
    ClassCompiler *class;
    ObjFunction *function;
    FunctionType type;

    Local *locals; //TODO(Skyler): Grow this.
    int localCount;
    Upvalue *upvalues; //TODO(Skyler): Grow this.
    int scopeDepth;
    uint16_t currentLibName;
    ObjScript *currentScript;
    Loop *loop;
    bool isWithBlock;
    char *withVarName;
} Compiler;

ObjFunction *compile(VM *vm, ObjScript *script, const char *source);
void markCompilerRoots(VM *vm);

#endif //__C_COMPILER_H__
