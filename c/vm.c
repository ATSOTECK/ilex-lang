//
// Created by Skyler on 1/10/22.
//

#include "vm.h"

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

static void runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    int line = getLine(vm.chunk, (int)(vm.ip - vm.chunk->code - 1));
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.globals);
    initTable(&vm.strings);
}

void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
}

// TODO(Skyler): Grow the stack.
void push(Value v) {
    *vm.stackTop = v;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

Value peek(int amount) {
    return vm.stackTop[-1 - amount];
}

static bool isFalsey(Value value) {
    return IS_NULL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concat() {
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int len = a->len + b->len;
    char* str = ALLOCATE(char, len + 1);
    memcpy(str, a->str, a->len);
    memcpy(str + a->len, b->str, b->len);
    str[len] = '\0';

    ObjString *res = takeString(str, len);
    push(OBJ_VAL(res));
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        runtimeError("Operands must be numbers."); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(valueType(a op b)); \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
            } break;
            case OP_NULL:  push(NULL_VAL); break;
            case OP_TRUE:  push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_POP: pop(); break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(vm.stack[slot]);
            } break;
            case OP_GET_GLOBAL: {
                ObjString *name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->str);

                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
            } break;
            case OP_DEFINE_GLOBAL: {
                ObjString *name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
            } break;
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = peek(0);
            } break;
            case OP_SET_GLOBAL: {
                ObjString *name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_EQ: {
                Value a = pop();
                Value b = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
            } break;
            case OP_NOTEQ: {
                Value a = pop();
                Value b = pop();
                push(BOOL_VAL(!valuesEqual(a, b)));
            } break;
            case OP_GR: BINARY_OP(BOOL_VAL, >); break;
            case OP_GREQ: BINARY_OP(BOOL_VAL, >=); break;
            case OP_LT: BINARY_OP(BOOL_VAL, <); break;
            case OP_LTEQ: BINARY_OP(BOOL_VAL, <=); break;
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concat();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_SUB: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MUL: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIV: BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT: push(BOOL_VAL(isFalsey(pop()))); break;
            case OP_NEG: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
            } break;
            case OP_PRINT: {
                printValue(pop());
                printf("\n");
            } break;
            case OP_RETURN: {
                return INTERPRET_OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char *source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult res = run();

    freeChunk(&chunk);
    return res;
}
