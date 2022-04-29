//
// Created by Skyler on 1/10/22.
//

#include "vm.h"

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"

#include "libs/lib_natives.h"
#include "libs/lib_string.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void resetStack(VM *vm) {
    vm->stackTop = vm->stack;
    vm->frameCount = 0;
    vm->openUpvalues = NULL;
}

void runtimeError(VM *vm, const char *format, ...) {
    fprintf(stderr, "\033[31mRuntime Error:\033[m ");

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm->frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm->frames[i];
        ObjFunction *function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        int line = function->chunk.lines[instruction];
        fprintf(stderr, "[line %d] in ", line);
        if (function->name == NULL) {
            fprintf(stderr, "script %s\n", vm->scriptName->str);
        } else {
            fprintf(stderr, "function %s()\n", function->name->str);
        }
    }

    resetStack(vm);
}

void assertError(VM *vm, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm->frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm->frames[i];
        ObjFunction *function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        int line = function->chunk.lines[instruction];
        fprintf(stderr, "[line %d] in ", line);
        if (function->name == NULL) {
            fprintf(stderr, "script %s\n", vm->scriptName->str);
        } else {
            fprintf(stderr, "function %s()\n", function->name->str);
        }
    }

    resetStack(vm);
}

void defineNative(VM *vm, const char *name, NativeFn function, Table *table) {
    push(vm, OBJ_VAL(copyString(vm, name, (int)strlen(name))));
    push(vm, OBJ_VAL(newNative(vm, function)));
    tableSet(vm, table, AS_STRING(vm->stack[0]), vm->stack[1]);
    pop(vm);
    pop(vm);
}

VM *initVM(const char *path) {
    VM *vm = (VM*)calloc(1, sizeof(VM));

    resetStack(vm);
    vm->objects = NULL;
    vm->bytesAllocated = 0;
    vm->nextGC = 1024 * 1024;
    vm->grayCount = 0;
    vm->grayCapacity = 0;
    vm->grayStack = NULL;

    initTable(&vm->globals);
    initTable(&vm->consts);
    initTable(&vm->strings);

    initTable(&vm->stringFunctions);

    vm->initString = NULL;
    vm->scriptName = NULL;
    vm->initString = copyString(vm, "init", 4);
    vm->scriptName = copyString(vm, path, (int)strlen(path));

    defineNatives(vm);
    defineStringFunctions(vm);

    return vm;
}

void freeVM(VM *vm) {
    freeTable(vm, &vm->globals);
    freeTable(vm, &vm->consts);
    freeTable(vm, &vm->strings);
    freeTable(vm, &vm->stringFunctions);
    vm->initString = NULL;
    vm->scriptName = NULL;
    freeObjects(vm);
}

// TODO(Skyler): Grow the stack.
void push(VM *vm, Value v) {
    *vm->stackTop = v;
    vm->stackTop++;
}

Value pop(VM *vm) {
    vm->stackTop--;
    return *vm->stackTop;
}

Value peek(VM *vm, int amount) {
    return vm->stackTop[-1 - amount];
}

static bool call(VM *vm, ObjClosure *closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError(vm ,"Expected %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }

    if (vm->frameCount == FRAMES_MAX) {
        runtimeError(vm, "Stack overflow.");
        return false;
    }

    CallFrame *frame = &vm->frames[vm->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm->stackTop - argCount - 1;

    return true;
}

static bool callValue(VM *vm, Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod *bound = AS_BOUND_METHOD(callee);
                vm->stackTop[-argCount - 1] = bound->receiver;
                return call(vm, bound->method, argCount);
            }
            case OBJ_CLASS: {
                ObjClass *objClass = AS_CLASS(callee);
                vm->stackTop[-argCount - 1] = OBJ_VAL(newInstance(vm, objClass));

                Value initializer;
                if (tableGet(&objClass->methods, vm->initString, &initializer)) {
                    return call(vm, AS_CLOSURE(initializer), argCount);
                } else if (argCount != 0) {
                    runtimeError(vm, "Expected 0 arguments but got %d.", argCount);
                    return false;
                }

                return true;
            }
            // TODO: OBJ_FUNCTION
            case OBJ_CLOSURE: return call(vm, AS_CLOSURE(callee), argCount);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(vm, argCount, vm->stackTop - argCount);
                vm->stackTop -= argCount + 1;
                push(vm, result);

                return true;
            }
            default: break; // Non-callable object type.
        }
    }

    runtimeError(vm, "Can only call functions and classes.");
    return false;
}

static bool callNativeFunction(VM *vm, Value function, int argc) {
    NativeFn native = AS_NATIVE(function);

    Value res = native(vm, argc, vm->stackTop - argc - 1);
    if (IS_EMPTY(res)) {
        return false;
    }

    vm->stackTop -= argc + 1;
    push(vm, res);
    return true;
}

static bool invokeFromClass(VM *vm, ObjClass *objClass, ObjString *name, int argCount) {
    Value method;
    if (!tableGet(&objClass->methods, name, &method)) {
        runtimeError(vm, "Undefined property '%s'.", name->str);
        return false;
    }

    return call(vm, AS_CLOSURE(method), argCount);
}

static bool invoke(VM *vm, ObjString *name, int argCount) {
    Value receiver = peek(vm, argCount);

    switch (getObjType(receiver)) {
        case OBJ_INSTANCE: {
            ObjInstance *instance = AS_INSTANCE(receiver);

            Value value;
            if (tableGet(&instance->fields, name, &value)) {
                vm->stackTop[-argCount - 1] = value;
                return callValue(vm, value, argCount);
            }

            return invokeFromClass(vm, instance->objClass, name, argCount);
        }
        case OBJ_STRING: {
            Value value;
            if (tableGet(&vm->stringFunctions, name, &value)) {
                return callNativeFunction(vm, value, argCount);
            }

            runtimeError(vm, "String has no method %s().", name->str);
            return false;
        }
    }

    runtimeError(vm, "Only instances have methods.");
    return false;
}

static bool bindMethod(VM *vm, ObjClass *objClass, ObjString *name) {
    Value method;
    if (!tableGet(&objClass->methods, name, &method)) {
        runtimeError(vm, "Undefined property '%s'.", name->str);

        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(vm, peek(vm, 0), AS_CLOSURE(method));
    pop(vm);
    push(vm, OBJ_VAL(bound));

    return true;
}

static ObjUpvalue *captureUpvalue(VM *vm, Value *local) {
    ObjUpvalue *prevUpvalue = NULL;
    ObjUpvalue *upvalue = vm->openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue *createdUpvalue = newUpvalue(vm, local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        vm->openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(VM *vm, Value *last) {
    while (vm->openUpvalues != NULL && vm->openUpvalues->location >= last) {
        ObjUpvalue *upvalue = vm->openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm->openUpvalues = upvalue->next;
    }
}

static void defineMethod(VM *vm, ObjString *name) {
    Value method = peek(vm, 0);
    ObjClass *objClass = AS_CLASS(peek(vm, 1));
    tableSet(vm, &objClass->methods, name, method);
    pop(vm);
}

static bool isFalsy(Value value) {
    return IS_NULL(value) ||
          (IS_BOOL(value) && !AS_BOOL(value)) ||
          (IS_NUMBER(value) && AS_NUMBER(value) == 0) ||
          (IS_STRING(value) && AS_STRING(value)->len == 0);
}

static void concat(VM *vm) {
    ObjString* b = AS_STRING(peek(vm, 0));
    ObjString* a = AS_STRING(peek(vm, 1));

    int len = a->len + b->len;
    char* str = ALLOCATE(vm, char, len + 1);
    memcpy(str, a->str, a->len);
    memcpy(str + a->len, b->str, b->len);
    str[len] = '\0';

    ObjString *res = takeString(vm, str, len);
    pop(vm);
    pop(vm);
    push(vm, OBJ_VAL(res));
}

static InterpretResult run(VM *vm) {
    CallFrame *frame = &vm->frames[vm->frameCount - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op) \
    do { \
      if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) { \
        runtimeError(vm, "Operands must be numbers."); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      double b = AS_NUMBER(pop(vm)); \
      double a = AS_NUMBER(pop(vm)); \
      push(vm, valueType(a op b)); \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(&frame->closure->function->chunk, (int)(frame->ip - frame->closure->function->chunk.code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(vm, constant);
            } break;
            case OP_NULL:  push(vm, NULL_VAL); break;
            case OP_TRUE:  push(vm, BOOL_VAL(true)); break;
            case OP_FALSE: push(vm, BOOL_VAL(false)); break;
            case OP_POP: pop(vm); break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(vm, frame->slots[slot]);
            } break;
            case OP_GET_GLOBAL: {
                ObjString *name = READ_STRING();
                Value value;
                if (!tableGet(&vm->globals, name, &value)) {
                    runtimeError(vm, "Undefined variable '%s'.", name->str);

                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, value);
            } break;
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(vm, *frame->closure->upvalues[slot]->location);
            } break;
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(vm, 0))) {
                    runtimeError(vm, "Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(vm, 0));
                ObjString *name = READ_STRING();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(vm); // Instance.
                    push(vm, value);
                    break;
                }

                if (!bindMethod(vm, instance->objClass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_GET_PROPERTY_NO_POP: {
                if (!IS_INSTANCE(peek(vm, 0))) {
                    runtimeError(vm, "Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
    
                ObjInstance *instance = AS_INSTANCE(peek(vm, 0));
                ObjString *name = READ_STRING();
                Value value;
    
                if (tableGet(&instance->fields, name, &value)) {
                    push(vm, value);
                    break;
                }
    
                if (!bindMethod(vm, instance->objClass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_GET_SUPER: {
                ObjString *name = READ_STRING();
                ObjClass *superclass = AS_CLASS(pop(vm));

                if (!bindMethod(vm, superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_DEFINE_GLOBAL: {
                ObjString *name = READ_STRING();
                tableSet(vm, &vm->globals, name, peek(vm, 0));
                pop(vm);
            } break;
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(vm, 0);
            } break;
            case OP_SET_GLOBAL: {
                ObjString *name = READ_STRING();
                if (tableSet(vm, &vm->globals, name, peek(vm, 0))) {
                    tableDelete(&vm->globals, name);
                    runtimeError(vm, "Undefined variable '%s'.", name->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(vm, 0);
            } break;
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(vm, 1))) {
                    runtimeError(vm, "Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(vm, 1));
                tableSet(vm, &instance->fields, READ_STRING(), peek(vm, 0));
                Value value = pop(vm);
                pop(vm);  // Instance.
                push(vm, value);
            } break;
            case OP_EQ: {
                Value a = pop(vm);
                Value b = pop(vm);
                push(vm, BOOL_VAL(valuesEqual(a, b)));
            } break;
            case OP_NOTEQ: {
                Value a = pop(vm);
                Value b = pop(vm);
                push(vm, BOOL_VAL(!valuesEqual(a, b)));
            } break;
            case OP_GR: BINARY_OP(BOOL_VAL, >); break;
            case OP_GREQ: BINARY_OP(BOOL_VAL, >=); break;
            case OP_LT: BINARY_OP(BOOL_VAL, <); break;
            case OP_LTEQ: BINARY_OP(BOOL_VAL, <=); break;
            case OP_ADD: {
                if (IS_NUMBER(peek(vm, 0)) && IS_NUMBER(peek(vm, 1))) {
                    double b = AS_NUMBER(pop(vm));
                    double a = AS_NUMBER(pop(vm));
                    push(vm, NUMBER_VAL(a + b));
                } else if (IS_STRING(peek(vm, 0)) && IS_STRING(peek(vm, 1))) {
                    concat(vm);
                } else {
                    runtimeError(vm, "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_INC: {
                if (!IS_NUMBER(peek(vm, 0))) {
                    runtimeError(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                push(vm, NUMBER_VAL(AS_NUMBER(pop(vm)) + 1));
            } break;
            case OP_SUB: BINARY_OP(NUMBER_VAL, -); break;
            case OP_DEC: {
                if (!IS_NUMBER(peek(vm, 0))) {
                    runtimeError(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
        
                push(vm, NUMBER_VAL(AS_NUMBER(pop(vm)) - 1));
            } break;
            case OP_MUL: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIV: BINARY_OP(NUMBER_VAL, /); break;
            case OP_POW: {
                if (!IS_NUMBER(peek(vm, 0) || !IS_NUMBER(peek(vm, 1)))) {
                    runtimeError(vm, "Operands must be two numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop(vm));
                double a = AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL(powf(a, b)));
            } break;
            case OP_MOD: {
                if (!IS_NUMBER(peek(vm, 0) || !IS_NUMBER(peek(vm, 1)))) {
                    runtimeError(vm, "Operands must be two numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop(vm));
                double a = AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL(fmod(a, b)));
            } break;
            case OP_NULL_COALESCE: {
                if (IS_NULL(peek(vm, 1))) {
                    Value rhs = pop(vm); // rhs
                    pop(vm); // lhs
                    push(vm, rhs);
                } else {
                    pop(vm); // rhs
                    push(vm, pop(vm)); // lhs
                }
            } break;
            case OP_OR: {
                if (isFalsy(peek(vm, 1))) {
                    Value rhs = pop(vm); // rhs
                    pop(vm); // lhs
                    push(vm, rhs);
                } else {
                    pop(vm); // rhs
                    push(vm, pop(vm)); // lhs
                }
            } break;
            case OP_NOT: push(vm, BOOL_VAL(isFalsy(pop(vm)))); break;
            case OP_NEG: {
                if (!IS_NUMBER(peek(vm, 0))) {
                    runtimeError(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, NUMBER_VAL(-AS_NUMBER(pop(vm))));
            } break;
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
            } break;
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsy(peek(vm, 0))) {
                    frame->ip += offset;
                }
            } break;
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
            } break;
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (!callValue(vm, peek(vm, argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm->frames[vm->frameCount - 1];
            } break;
            case OP_INVOKE: {
                ObjString *method = READ_STRING();
                int argCount = READ_BYTE();
                if (!invoke(vm, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm->frames[vm->frameCount - 1];
            } break;
            case OP_SUPER_INVOKE: {
                ObjString *method = READ_STRING();
                int argc = READ_BYTE();
                ObjClass *superclass = AS_CLASS(pop(vm));
                if (!invokeFromClass(vm, superclass, method, argc)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm->frames[vm->frameCount - 1];
            } break;
            case OP_CLOSURE: {
                ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure *closure = newClosure(vm, function);
                push(vm, OBJ_VAL(closure));

                for (int i = 0; i < closure->upvalueCount; ++i) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(vm, frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
            } break;
            case OP_CLOSE_UPVALUE: {
                closeUpvalues(vm, vm->stackTop - 1);
                pop(vm);
            } break;
            case OP_RETURN: {
                Value result = pop(vm);
                vm->frameCount--;
                closeUpvalues(vm, frame->slots);

                if (vm->frameCount == 0) {
                    pop(vm);
                    return INTERPRET_OK;
                }

                vm->stackTop = frame->slots;
                push(vm, result);
                frame = &vm->frames[vm->frameCount - 1];
            } break;
            case OP_CLASS: {
                push(vm, OBJ_VAL(newClass(vm, READ_STRING())));
            } break;
            case OP_INHERIT: {
                Value superclass = peek(vm, 1);
                if (!IS_CLASS(superclass)) {
                    runtimeError(vm, "Superclass must be a class.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjClass *subclass = AS_CLASS(peek(vm, 0));
                tableAddAll(vm, &AS_CLASS(superclass)->methods, &subclass->methods);
                pop(vm); // Subclass.
            } break;
            case OP_METHOD: {
                defineMethod(vm, READ_STRING());
            }break;
            case OP_ASSERT: {
                Value condition = pop(vm);
                ObjString *error = READ_STRING();

                if (isFalsy(condition)) {
                    if (!error->str[0]) {
                        assertError(vm, "Assertion Failed.");
                    } else {
                        assertError(vm, "Assertion failed with message: %s", error->str);
                    }
                    
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_MULTI_CASE: {
                int count = READ_BYTE();
                Value switchValue = peek(vm, count + 1);
                Value caseValue = pop(vm);
                for (int i = 0; i < count; ++i) {
                    if (valuesEqual(switchValue, caseValue)) {
                        i++;
                        while(i <= count) {
                            pop(vm);
                            i++;
                        } break;
                    }
                    caseValue = pop(vm);
                }
                push(vm,caseValue);
            } break;
            case OP_CMP_JMP: {
                uint16_t offset = READ_SHORT();
                Value a = pop(vm);
                if (!valuesEqual(peek(vm,0), a)) {
                    frame->ip += offset;
                } else {
                    pop(vm); // switch expression.
                }
            } break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(VM *vm, const char *source) {
    ObjFunction *function = compile(vm, source);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    push(vm, OBJ_VAL(function));
    ObjClosure *closure = newClosure(vm, function);
    pop(vm);
    push(vm, OBJ_VAL(closure));
    call(vm, closure, 0);

    return run(vm);
}
