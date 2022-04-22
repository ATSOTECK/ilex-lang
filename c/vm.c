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
#include <string.h>

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

void runtimeError(const char *format, ...) {
    fprintf(stderr, "Runtime Error: ");

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        int line = function->chunk.lines[instruction];
        fprintf(stderr, "[line %d] in ", line);
        if (function->name == NULL) {
            fprintf(stderr, "script %s\n", vm.scriptName->str);
        } else {
            fprintf(stderr, "function %s()\n", function->name->str);
        }
    }

    resetStack();
}

void assertError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        int line = function->chunk.lines[instruction];
        fprintf(stderr, "[line %d] in ", line);
        if (function->name == NULL) {
            fprintf(stderr, "script %s\n", vm.scriptName->str);
        } else {
            fprintf(stderr, "function %s()\n", function->name->str);
        }
    }

    resetStack();
}

void defineNative(const char *name, NativeFn function, Table *table) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(table, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM(const char *path) {
    resetStack();
    vm.objects = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;
    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;

    initTable(&vm.globals);
    initTable(&vm.consts);
    initTable(&vm.strings);

    initTable(&vm.stringFunctions);

    vm.initString = NULL;
    vm.scriptName = NULL;
    vm.initString = copyString("init", 4);
    vm.scriptName = copyString(path, (int)strlen(path));

    defineNatives(&vm);
    defineStringFunctions(&vm);
}

void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.consts);
    freeTable(&vm.strings);
    freeTable(&vm.stringFunctions);
    vm.initString = NULL;
    vm.scriptName = NULL;
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

static bool call(ObjClosure *closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;

    return true;
}

static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod *bound = AS_BOUND_METHOD(callee);
                vm.stackTop[-argCount - 1] = bound->receiver;
                return call(bound->method, argCount);
            }
            case OBJ_CLASS: {
                ObjClass *objClass = AS_CLASS(callee);
                vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(objClass));

                Value initializer;
                if (tableGet(&objClass->methods, vm.initString, &initializer)) {
                    return call(AS_CLOSURE(initializer), argCount);
                } else if (argCount != 0) {
                    runtimeError("Expected 0 arguments but got %d.", argCount);
                    return false;
                }

                return true;
            }
            // TODO: OBJ_FUNCTION
            case OBJ_CLOSURE: return call(AS_CLOSURE(callee), argCount);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(&vm, argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);

                return true;
            }
            default: break; // Non-callable object type.
        }
    }

    runtimeError("Can only call functions and classes.");
    return false;
}

static bool callNativeFunction(Value function, int argc) {
    NativeFn native = AS_NATIVE(function);

    Value res = native(&vm, argc, vm.stackTop - argc - 1);
    if (IS_EMPTY(res)) {
        return false;
    }

    vm.stackTop -= argc + 1;
    push(res);
    return true;
}

static bool invokeFromClass(ObjClass *objClass, ObjString *name, int argCount) {
    Value method;
    if (!tableGet(&objClass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->str);
        return false;
    }

    return call(AS_CLOSURE(method), argCount);
}

static bool invoke(ObjString *name, int argCount) {
    Value receiver = peek(argCount);

    switch (getObjType(receiver)) {
        case OBJ_INSTANCE: {
            ObjInstance *instance = AS_INSTANCE(receiver);

            Value value;
            if (tableGet(&instance->fields, name, &value)) {
                vm.stackTop[-argCount - 1] = value;
                return callValue(value, argCount);
            }

            return invokeFromClass(instance->objClass, name, argCount);
        }
        case OBJ_STRING: {
            Value value;
            if (tableGet(&vm.stringFunctions, name, &value)) {
                return callNativeFunction(value, argCount);
            }

            runtimeError("String has no method %s().", name->str);
            return false;
        }
    }

    runtimeError("Only instances have methods.");
    return false;
}

static bool bindMethod(ObjClass *objClass, ObjString *name) {
    Value method;
    if (!tableGet(&objClass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->str);

        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJ_VAL(bound));

    return true;
}

static ObjUpvalue *captureUpvalue(Value *local) {
    ObjUpvalue *prevUpvalue = NULL;
    ObjUpvalue *upvalue = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue *createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(Value *last) {
    while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue *upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

static void defineMethod(ObjString *name) {
    Value method = peek(0);
    ObjClass *objClass = AS_CLASS(peek(1));
    tableSet(&objClass->methods, name, method);
    pop();
}

static bool isFalsey(Value value) {
    return IS_NULL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concat() {
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));

    int len = a->len + b->len;
    char* str = ALLOCATE(char, len + 1);
    memcpy(str, a->str, a->len);
    memcpy(str + a->len, b->str, b->len);
    str[len] = '\0';

    ObjString *res = takeString(str, len);
    pop();
    pop();
    push(OBJ_VAL(res));
}

static InterpretResult run() {
    CallFrame *frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
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
        disassembleInstruction(&frame->closure->function->chunk, (int)(frame->ip - frame->closure->function->chunk.code));
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
                push(frame->slots[slot]);
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
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
            } break;
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(0));
                ObjString *name = READ_STRING();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(); // Instance.
                    push(value);
                    break;
                }

                if (!bindMethod(instance->objClass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_GET_PROPERTY_NO_POP: {
                if (!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
    
                ObjInstance *instance = AS_INSTANCE(peek(0));
                ObjString *name = READ_STRING();
                Value value;
    
                if (tableGet(&instance->fields, name, &value)) {
                    push(value);
                    break;
                }
    
                if (!bindMethod(instance->objClass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_GET_SUPER: {
                ObjString *name = READ_STRING();
                ObjClass *superclass = AS_CLASS(pop());

                if (!bindMethod(superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_DEFINE_GLOBAL: {
                ObjString *name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
            } break;
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
            } break;
            case OP_SET_GLOBAL: {
                ObjString *name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
            } break;
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(1))) {
                    runtimeError("Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(1));
                tableSet(&instance->fields, READ_STRING(), peek(0));
                Value value = pop();
                pop();  // Instance.
                push(value);
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
                if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concat();
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_INC: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                push(NUMBER_VAL(AS_NUMBER(pop()) + 1));
            } break;
            case OP_SUB: BINARY_OP(NUMBER_VAL, -); break;
            case OP_DEC: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
        
                push(NUMBER_VAL(AS_NUMBER(pop()) - 1));
            } break;
            case OP_MUL: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIV: BINARY_OP(NUMBER_VAL, /); break;
            case OP_POW: {
                if (!IS_NUMBER(peek(0) || !IS_NUMBER(peek(1)))) {
                    runtimeError("Operands must be two numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(powf(a, b)));
            } break;
            case OP_MOD: {
                if (!IS_NUMBER(peek(0) || !IS_NUMBER(peek(1)))) {
                    runtimeError("Operands must be two numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(fmod(a, b)));
            } break;
            case OP_NOT: push(BOOL_VAL(isFalsey(pop()))); break;
            case OP_NEG: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
            } break;
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
            } break;
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) {
                    frame->ip += offset;
                }
            } break;
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
            } break;
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
            } break;
            case OP_INVOKE: {
                ObjString *method = READ_STRING();
                int argCount = READ_BYTE();
                if (!invoke(method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm.frames[vm.frameCount - 1];
            } break;
            case OP_SUPER_INVOKE: {
                ObjString *method = READ_STRING();
                int argc = READ_BYTE();
                ObjClass *superclass = AS_CLASS(pop());
                if (!invokeFromClass(superclass, method, argc)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm.frames[vm.frameCount - 1];
            } break;
            case OP_CLOSURE: {
                ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure *closure = newClosure(function);
                push(OBJ_VAL(closure));

                for (int i = 0; i < closure->upvalueCount; ++i) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
            } break;
            case OP_CLOSE_UPVALUE: {
                closeUpvalues(vm.stackTop - 1);
                pop();
            } break;
            case OP_RETURN: {
                Value result = pop();
                vm.frameCount--;
                closeUpvalues(frame->slots);

                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
            } break;
            case OP_CLASS: {
                push(OBJ_VAL(newClass(READ_STRING())));
            } break;
            case OP_INHERIT: {
                Value superclass = peek(1);
                if (!IS_CLASS(superclass)) {
                    runtimeError("Superclass must be a class.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjClass *subclass = AS_CLASS(peek(0));
                tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
                pop(); // Subclass.
            } break;
            case OP_METHOD: {
                defineMethod(READ_STRING());
            }break;
            case OP_ASSERT: {
                Value condition = pop();
                ObjString *error = READ_STRING();

                if (isFalsey(condition)) {
                    if (!error->str[0]) {
                        assertError("Assertion Failed.");
                    } else {
                        assertError("Assertion failed with message: %s", error->str);
                    }
                    
                    return INTERPRET_RUNTIME_ERROR;
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

InterpretResult interpret(const char *source, const char *path) {
    initVM(path);

    ObjFunction *function = compile(source);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    push(OBJ_VAL(function));
    ObjClosure *closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure, 0);

    return run();
}
