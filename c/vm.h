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

struct VM_ {
    Compiler *compiler;
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    Value stack[STACK_MAX];
    Value *stackTop;
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

    size_t bytesAllocated;
    size_t nextGC;
    size_t gcRuns;
    Obj *objects;
    int grayCount;
    int grayCapacity;
    Obj** grayStack;
    ObjString *scriptName;

    bool envLoaded;
    bool fallThrough;
};

typedef enum {
    INTERPRET_GOOD          = (int)0x0B00B135,
    INTERPRET_COMPILE_ERROR = (int)0xBAADF00D,
    INTERPRET_RUNTIME_ERROR = (int)0xDEADDEAD,
    INTERPRET_ASSERT_ERROR  = (int)0xBAADC0DE,
    INTERPRET_PANIC_ERROR   = (int)0xBAAAAAAD
} InterpretResult;

VM *initVM(const char *path);
void freeVM(VM *vm);

void runtimeError(VM *vm, const char *format, ...);
InterpretResult interpret(VM *vm, const char *scriptName, const char *source);
void defineNative(VM *vm, const char *name, NativeFn function, Table *table);
void defineNativeValue(VM *vm, const char *name, Value value, Table *table);

void push(VM *vm, Value v);
Value pop(VM *vm);

Value callFromScript(VM *vm, ObjClosure *closure, int argc, Value *args);

InterpretResult run(VM *vm, int frameIndex, Value *value);

#endif //C_VM_H
