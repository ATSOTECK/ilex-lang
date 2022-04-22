//
// Created by Skyler on 1/10/22.
//

#ifndef C_VM_H
#define C_VM_H

#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    ObjClosure *closure;
    uint8_t *ip;
    Value *slots;
} CallFrame;

struct VM_ {
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    Value stack[STACK_MAX];
    Value *stackTop;
    Table globals;
    Table consts;
    Table strings;
    ObjString *initString;
    ObjUpvalue *openUpvalues;

    Table stringFunctions;

    size_t bytesAllocated;
    size_t nextGC;
    Obj *objects;
    int grayCount;
    int grayCapacity;
    Obj** grayStack;
    ObjString *scriptName;
};

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM(const char *path);
void freeVM();

void runtimeError(const char *format, ...);
InterpretResult interpret(const char *source, const char *path);
void defineNative(const char *name, NativeFn function, Table *table);

void push(Value v);
Value pop();

#endif //C_VM_H
