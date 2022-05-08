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

    size_t bytesAllocated;
    size_t nextGC;
    Obj *objects;
    int grayCount;
    int grayCapacity;
    Obj** grayStack;
    ObjString *scriptName;
};

typedef enum {
    INTERPRET_GOOD          = 0x0B00B135,
    INTERPRET_COMPILE_ERROR = 0xBAADF00D,
    INTERPRET_RUNTIME_ERROR = 0xDEADDEAD,
    INTERPRET_ASSERT_ERROR  = 0xBAADC0DE,
    INTERPRET_PANIC_ERROR   = 0xBAAAAAAD
} InterpretResult;

VM *initVM(const char *path);
void freeVM(VM *vm);

void runtimeError(VM *vm, const char *format, ...);
InterpretResult interpret(VM *vm, const char *source);
void defineNative(VM *vm, const char *name, NativeFn function, Table *table);
void defineNativeValue(VM *vm, const char *name, Value value, Table *table);

void push(VM *vm, Value v);
Value pop(VM *vm);

#endif //C_VM_H
