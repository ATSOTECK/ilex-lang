//
// Created by Skyler on 1/10/22.
//

#ifndef C_VM_H
#define C_VM_H

#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "compiler.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    ObjClosure *closure;
    uint8_t *ip;
    Value *slots;
} CallFrame;

typedef struct {
    char *name;
    BuiltInLib lib;
} BuiltInLibs;

struct VM_ {
    Compiler *compiler;
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    Value *stack;
    Value *stackTop;
    int stackHeight;
    Table globals;
    Table consts;
    Table strings;
    ObjString *initString;
    ObjUpvalue *openUpvalues;

    Table scripts;
    ObjScript *lastScript; // Used for 'from'.
    Table stringFunctions;
    Table arrayFunctions;
    Table fileFunctions;
    Table mapFunctions;
    Table setFunctions;

    size_t bytesAllocated;
    size_t nextGC;
    size_t gcRuns;
    Obj *objects;
    int grayCount;
    int grayCapacity;
    Obj** grayStack;
    ObjString *scriptName;

    int fnCount;
    int valCount;

    bool envLoaded;
    bool fallThrough;

    BuiltInLibs *libs;
    int libCount;
    int libCapacity;
    
    int argc;
    char **argv;
    
    ErrorCallback runtimeCallback;
    ErrorCallback assertCallback;
    ErrorCallback panicCallback;
};

InterpretResult interpret(VM *vm, const char *scriptName, const char *source);
void defineNative(VM *vm, const char *name, NativeFn function, Table *table);
void defineNativeValue(VM *vm, const char *name, Value value, Table *table);

void push(VM *vm, Value v);
Value pop(VM *vm);

Value callFromScript(VM *vm, ObjClosure *closure, int argc, Value *args);

InterpretResult run(VM *vm, int frameIndex, Value *value);

#endif //C_VM_H
