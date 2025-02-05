//
// Created by Skyler on 1/10/22.
//

#include "vm.h"

#include "ilex.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "util.h"

#include "types/type_array.h"
#include "libs/lib_builtIn.h"
#include "types/type_enum.h"
#include "types/type_file.h"
#include "types/type_map.h"
#include "libs/lib_natives.h"
#include "types/type_set.h"
#include "types/type_string.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void resetStack(VM *vm) {
    vm->stackTop = vm->stack;
    vm->frameCount = 0;
#ifdef DEBUG_MODE
    vm->stackHeight = 0;
#endif
    vm->openUpvalues = NULL;
    vm->compiler = NULL;
}

#ifdef DEBUG_MODE
void printStack(VM *vm) {
    printf("vvvvvvvvvvvvv\n");
    Value *sp = vm->stackTop - 1;
    for (int i = 0; i < vm->stackHeight; ++i) {
        printValueNl(*sp);
        --sp;
    }
    printf("^^^^^^^^^^^^\n");
}
#endif

void setRuntimeErrorCallback(VM *vm, ErrorCallback runtimeCallback) {
    vm->runtimeCallback = runtimeCallback;
}

void setAssertErrorCallback(VM *vm, ErrorCallback assertCallback) {
    vm->assertCallback = assertCallback;
}

void setPanicErrorCallback(VM *vm, ErrorCallback panicCallback) {
    vm->panicCallback = panicCallback;
}

void runtimeError(VM *vm, const char *format, ...) {
    char *msg = (char*)malloc(sizeof(char) * I_ERR_MSG_SIZE);
    int len;
    if (vm->runtimeCallback != NULL) {
        len = snprintf(msg, I_ERR_MSG_SIZE, "Runtime Error: ");
    } else {
        len = snprintf(msg, I_ERR_MSG_SIZE, "\033[31mRuntime Error:\033[m ");
    }
    
    va_list args;
    va_start(args, format);
    len += vsnprintf(msg + len, I_ERR_MSG_SIZE, format, args);
    va_end(args);
    msg[len++] = '\n';

    for (int i = vm->frameCount - 1; i >= 0; --i) {
        CallFrame *frame = &vm->frames[i];
        ObjFunction *function = frame->closure->function;
        // TODO: Find a better way to store line numbers.
        size_t instruction = frame->ip - function->chunk.code - 1;
        int line = function->chunk.lines[instruction];
        len += snprintf(msg + len, I_ERR_MSG_SIZE, "[line %d] in ", line);
        if (function->name == NULL) {
            len += snprintf(msg + len, I_ERR_MSG_SIZE, "script %s\n", function->script->name->str);
            i = -1;
        } else {
            len += snprintf(msg + len, I_ERR_MSG_SIZE, "function '%s' in script %s\n", function->name->str, function->script->name->str);
        }
    }
    
    msg[len] = '\0';
    if (vm->runtimeCallback != NULL) {
        vm->runtimeCallback(msg);
    } else {
        fprintf(stderr, "%s", msg);
    }
    
    free(msg);
    resetStack(vm);
}

void assertError(VM *vm, const char *format, ...) {
    char *msg = (char*)malloc(sizeof(char) * I_ERR_MSG_SIZE);
    va_list args;
    va_start(args, format);
    int len = vsnprintf(msg, I_ERR_MSG_SIZE, format, args);
    va_end(args);
    msg[len++] = '\n';

    for (int i = vm->frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm->frames[i];
        ObjFunction *function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        int line = function->chunk.lines[instruction];
        len += snprintf(msg + len, I_ERR_MSG_SIZE, "[line %d] in ", line);
        if (function->name == NULL) {
            len += snprintf(msg + len, I_ERR_MSG_SIZE, "script %s\n", vm->scriptName->str);
        } else {
            len += snprintf(msg + len, I_ERR_MSG_SIZE, "function %s()\n", function->name->str);
        }
    }
    
    if (vm->assertCallback != NULL) {
        vm->assertCallback(msg);
    } else {
        fprintf(stderr, "%s", msg);
    }

    free(msg);
    resetStack(vm);
}

void panicError(VM *vm, const char *panicMsg) {
    char *msg = (char*)malloc(sizeof(char) * I_ERR_MSG_SIZE);
    int len;
    if (vm->panicCallback != NULL) {
        len = snprintf(msg, I_ERR_MSG_SIZE, "Panic! %s\n", panicMsg);
    } else {
        len = snprintf(msg, I_ERR_MSG_SIZE, "\033[31mPanic!\033[m %s\n", panicMsg);
    }

    for (int i = vm->frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm->frames[i];
        ObjFunction *function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        int line = function->chunk.lines[instruction];
        len += snprintf(msg + len, I_ERR_MSG_SIZE, "[line %d] in ", line);
        if (function->name == NULL) {
            len += snprintf(msg + len, I_ERR_MSG_SIZE, "script %s\n", vm->scriptName->str);
        } else {
            len += snprintf(msg + len, I_ERR_MSG_SIZE, "function %s()\n", function->name->str);
        }
    }
    
    if (vm->panicCallback != NULL) {
        vm->panicCallback(msg);
    } else {
        fprintf(stderr, "%s", msg);
    }

    free(msg);
    resetStack(vm);
}

void defineNative(VM *vm, const char *name, NativeFn function, Table *table) {
    ObjString *nativeName = copyString(vm, name, (int)strlen(name));
    push(vm, OBJ_VAL(nativeName));
    ObjNative *nativeFunction = newNative(vm, function);
    push(vm, OBJ_VAL(nativeFunction));
    tableSet(vm, table, nativeName, OBJ_VAL(nativeFunction), ILEX_READ_ONLY);
    pop(vm);
    pop(vm);
    ++vm->fnCount;
}

void registerGlobalFunction(VM *vm, const char *name, NativeFn function) {
    defineNative(vm, name, function, &vm->globals);
}

void defineNativeValue(VM *vm, const char *name, Value value, Table *table) {
    ObjString *valueName = copyString(vm, name, (int)strlen(name));
    push(vm, OBJ_VAL(valueName));
    push(vm, value);
    tableSet(vm, table, valueName, value, ILEX_READ_ONLY);
    pop(vm);
    pop(vm);
    ++vm->valCount;
}

void registerGlobalValue(VM *vm, const char *name, Value value) {
    defineNativeValue(vm, name, value, &vm->globals);
}

void registerLibraryFunction(VM *vm, const char *name, NativeFn function, Table *table) {
    defineNative(vm, name, function, table);
}

void registerLibrary(VM *vm, const char *name, BuiltInLib lib) {
    BuiltInLibs newLib = makeLib(vm, name, lib);
    if (vm->libCapacity < vm->libCount + 1) {
        int oldCapacity = vm->libCapacity;
        vm->libCapacity = GROW_CAPACITY(oldCapacity);
        vm->libs = GROW_ARRAY(vm, BuiltInLibs, vm->libs, oldCapacity, vm->libCapacity);
    }

    vm->libs[vm->libCount++] = newLib;
}

VM *initVM(const char *path, int argc, char **argv) {
    VM *vm = (VM*)calloc(1, sizeof(VM));

    vm->stack = (Value*)malloc(sizeof(Value) * STACK_MAX);
    resetStack(vm);
    vm->objects = NULL;
    vm->bytesAllocated = 0;
    vm->nextGC = 1024 * 1024;
    vm->gcRuns = 0;
    vm->grayCount = 0;
    vm->grayCapacity = 0;
    vm->grayStack = NULL;
    vm->lastScript = NULL;
    vm->runtimeCallback = NULL;
    vm->assertCallback = NULL;
    vm->panicCallback = NULL;

    vm->fnCount = 0;
    vm->valCount = 0;
    
    vm->argc = argc;
    vm->argv = argv;

    initTable(&vm->globals);
    initTable(&vm->consts);
    initTable(&vm->strings);

    initTable(&vm->scripts);
    initTable(&vm->stringFunctions);
    initTable(&vm->arrayFunctions);
    initTable(&vm->fileFunctions);
    initTable(&vm->mapFunctions);
    initTable(&vm->setFunctions);
    initTable(&vm->enumFunctions);

    vm->initString = NULL;
    vm->scriptName = NULL;
    vm->initString = copyString(vm, "init", 4);
    vm->scriptName = copyString(vm, path, (int)strlen(path));
    vm->envLoaded = false;
    vm->fallThrough = false;

    vm->window = nullptr;
    vm->testMode = (strcmp("test", argv[1]) == 0);

    defineNatives(vm);
    defineStringFunctions(vm);
    defineArrayFunctions(vm);
    defineFileFunctions(vm);
    defineMapFunctions(vm);
    defineSetFunctions(vm);
    defineEnumFunctions(vm);

    initBuiltInLibs(vm);

    return vm;
}

void freeVM(VM *vm) {
    freeTable(vm, &vm->globals);
    freeTable(vm, &vm->consts);
    freeTable(vm, &vm->strings);
    freeTable(vm, &vm->stringFunctions);
    freeTable(vm, &vm->arrayFunctions);
    freeTable(vm, &vm->fileFunctions);
    freeTable(vm, &vm->mapFunctions);
    freeTable(vm, &vm->setFunctions);
    freeTable(vm, &vm->enumFunctions);
    for (int i = 0; i < vm->libCount; ++i) {
        FREE(vm, char, vm->libs[i].name);
    }
    FREE(vm, BuiltInLibs, vm->libs);
    vm->initString = NULL;
    vm->scriptName = NULL;
    freeObjects(vm);
    free(vm->stack);
}

// TODO(Skyler): Grow the stack if needed.
void push(VM *vm, Value v) {
    *vm->stackTop = v;
    vm->stackTop++;
#ifdef DEBUG_MODE
    vm->stackHeight++;
#endif
}

Value pop(VM *vm) {
#ifdef DEBUG_MODE
    vm->stackHeight--;
#endif
    vm->stackTop--;
    return *vm->stackTop;
}

Value peek(VM *vm, int amount) {
    return vm->stackTop[-1 - amount];
}

Value callFromScript(VM *vm, ObjClosure *closure, int argc, Value *args) {
    if (argc < closure->function->arity ||
        argc > closure->function->arity + closure->function->arityDefault) {
        runtimeError(vm ,"Function '%s' expected %d arguments but got %d.", closure->function->name->str,
                     closure->function->arity + closure->function->arityDefault, argc);
        return ERROR_VAL;
    }

    if (vm->frameCount == FRAMES_MAX) {
        runtimeError(vm, "Stack overflow.");
        return ERROR_VAL;
    }

    int currentFrameIndex = vm->frameCount - 1;
    CallFrame *frame = &vm->frames[vm->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;

    // Push args onto the stack.
    for (int i = 0; i < argc; ++i) {
        push(vm, args[i]);
    }

    frame->slots = vm->stackTop - argc - 1;
    Value value;
    run(vm, currentFrameIndex, &value);

    // Pop args from the stack when done.
    for (int i = 0; i < argc; ++i) {
        pop(vm);
    }

    return value;
}

static bool call(VM *vm, ObjClosure *closure, int argc) {
    if (argc < closure->function->arity ||
        argc > closure->function->arity + closure->function->arityDefault)
    {
        runtimeError(vm ,"Function '%s' expected %d arguments but got %d.", closure->function->name->str,
                     closure->function->arity + closure->function->arityDefault, argc);
        return ERROR_VAL;
    }

    if (vm->frameCount == FRAMES_MAX) {
        runtimeError(vm, "Stack overflow. (FRAMES_MAX)");
        return false;
    }

    CallFrame *frame = &vm->frames[vm->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm->stackTop - argc - 1;

    return true;
}

static bool callValue(VM *vm, Value callee, int argc) {
#ifdef DEBUG_MODE
    if (callee == 0) {
        runtimeError(vm, "Empty value passed to callValue.");
        return false;
    }
#endif

    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod *bound = AS_BOUND_METHOD(callee);
                vm->stackTop[-argc - 1] = bound->receiver;
                return call(vm, bound->method, argc);
            }
            case OBJ_CLASS: {
                if (!(IS_DEFAULT_CLASS(callee))) {
                    break;
                }
                
                ObjClass *objClass = AS_CLASS(callee);
                vm->stackTop[-argc - 1] = OBJ_VAL(newInstance(vm, objClass));

                Value initializer;
                if (tableGet(&objClass->methods, vm->initString, &initializer)) {
                    return call(vm, AS_CLOSURE(initializer), argc);
                } else if (argc != 0) {
                    runtimeError(vm, "Expected 0 arguments but got %d.", argc);
                    return false;
                }

                return true;
            }
            // TODO: OBJ_FUNCTION
            case OBJ_CLOSURE: {
                vm->stackTop[-argc - 1] = callee;
                return call(vm, AS_CLOSURE(callee), argc);
            }
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(vm, argc, vm->stackTop - argc);

                if (IS_ERR(result)) {
                    return false;
                }

#ifdef DEBUG_MODE
                vm->stackHeight -= argc + 1;
#endif
                vm->stackTop -= argc + 1;
                push(vm, result);

                return true;
            }
            default: break; // Non-callable object type.
        }
    }
    
    runtimeError(vm, "Can only call functions and classes. Tried to call '%s' of type '%s'.", valueToString(callee), valueType(callee));
    return false;
}

static bool callNativeFunction(VM *vm, NativeFn native, int argc) {
    Value res = native(vm, argc, vm->stackTop - argc - 1);
    if (IS_ERR(res)) {
        return false;
    }

#ifdef DEBUG_MODE
    vm->stackHeight -= argc + 1;
#endif
    vm->stackTop -= argc + 1;
    push(vm, res);
    return true;
}

static bool invokeFromClass(VM *vm, ObjClass *objClass, ObjString *name, int argc) {
    Value method;
    if (!tableGet(&objClass->methods, name, &method)) {
        runtimeError(vm, "Undefined property '%s'.", name->str);
        return false;
    }

    return call(vm, AS_CLOSURE(method), argc);
}

static bool invokeFromThis(VM *vm, ObjString *name, int argc) {
    Value receiver = peek(vm, argc);

    if (IS_INSTANCE(receiver)) {
        ObjInstance *instance = AS_INSTANCE(receiver);

        Value value;
        if (tableGet(&instance->objClass->privateMethods, name, &value)) {
            return call(vm, AS_CLOSURE(value), argc);
        }

        if (tableGet(&instance->objClass->methods, name, &value)) {
            return call(vm , AS_CLOSURE(value), argc);
        }

        // TODO: Instance methods.
        /*
        if (tableGet(vm->instanceMethods, name, &value)) {
            return callNativeFunction(vm , AS_NATIVE(value), argc);
        }
        */

        if (tableGet(&instance->objClass->staticVars, name, &value)) {
            vm->stackTop[-argc - 1] = value;
            return callValue(vm, value, argc);
        }
    } else if (IS_CLASS(receiver)) {
        ObjClass *instance = AS_CLASS(receiver);
        Value value;
        if (tableGet(&instance->privateMethods, name, &value)) {
            if (AS_CLOSURE(value)->function->type != TYPE_STATIC) {
                // TODO
                /*
                if (tableGet(&vm->classMethods, name, &value)) {
                    return callNativeFunction(vm, AS_NATIVE(value), argc);
                }
                */

                runtimeError(vm, "'%s', is not static. Only static methods can be invoked from a class.", name->str);
                return false;
            }

            return callValue(vm, value, argc);
        }

        if (tableGet(&instance->methods, name, &value)) {
            if (AS_CLOSURE(value)->function->type != TYPE_STATIC) {
                // TODO: Static methods.
                /*
                if (tableGet(&vm->classMethods, name, &value)) {
                    return callNativeFunction(vm, AS_NATIVE(value), argc);
                }
                */

                runtimeError(vm, "'%s', is not static. Only static methods can be invoked from a class.", name->str);
                return false;
            }

            return callValue(vm, value, argc);
        }

        // TODO: Static methods.
        /*
        if (tableGet(&vm->classMethods, name, &value)) {
            callNativeFunction(vm, AS_NATIVE(value), argc);
        }
        */
    }

    runtimeError(vm, "Undefined property '%s'.", name->str);
    return false;
}

static bool invoke(VM *vm, ObjString *name, int argc) {
    Value receiver = peek(vm, argc);
    
    if (!IS_OBJ(receiver)) {
        char *type = valueType(receiver);
        runtimeError(vm, "Only objects have methods. Tried to run method on type '%s'.", type);
        free(type);
        return false;
    }

    switch (getObjType(receiver)) {
        case OBJ_INSTANCE: {
            ObjInstance *instance = AS_INSTANCE(receiver);

            Value value;
            if (tableGet(&instance->fields, name, &value)) {
                vm->stackTop[-argc - 1] = value;
                return callValue(vm, value, argc);
            }

            return invokeFromClass(vm, instance->objClass, name, argc);
        }
        case OBJ_STRING: {
            Value value;
            if (tableGet(&vm->stringFunctions, name, &value)) {
                return callNativeFunction(vm, AS_NATIVE(value), argc);
            }

            runtimeError(vm, "String has no function %s().", name->str);
            return false;
        }
        case OBJ_ENUM: {
            Value value;
            if (tableGet(&vm->enumFunctions, name, &value)) {
                return callNativeFunction(vm, AS_NATIVE(value), argc);
            }
            
            ObjEnum *enumObj = AS_ENUM(receiver);

            if (tableGet(&enumObj->values, name, &value)) {
                return callValue(vm, value, argc);
            }

            runtimeError(vm, "'%s' enum has no property '%s'.", enumObj->name->str, name->str);
            return false;
        }
        case OBJ_SCRIPT: {
            ObjScript *script = AS_SCRIPT(receiver);

            Value value;
            if (!tableGet(&script->values, name, &value)) {
                runtimeError(vm, "Undefined property '%s' on '%s'.", name->str, script->name->str);
                return false;
            }

            return callValue(vm, value, argc);
        }
        case OBJ_ARRAY: {
            Value value;
            if (tableGet(&vm->arrayFunctions, name, &value)) {
                return callNativeFunction(vm, AS_NATIVE(value), argc);
            }
    
            runtimeError(vm, "Array has no function %s().", name->str);
            return false;
        }
        case OBJ_FILE: {
            Value value;
            if (tableGet(&vm->fileFunctions, name, &value)) {
                return callNativeFunction(vm, AS_NATIVE(value), argc);
            }
    
            runtimeError(vm, "File has no function %s().", name->str);
            return false;
        }
        case OBJ_MAP: {
            Value value;
            if (tableGet(&vm->mapFunctions, name, &value)) {
                return callNativeFunction(vm, AS_NATIVE(value), argc);
            }
    
            runtimeError(vm, "Map has no function %s().", name->str);
            return false;
        }
        case OBJ_SET: {
            Value value;
            if (tableGet(&vm->setFunctions, name, &value)) {
                return callNativeFunction(vm, AS_NATIVE(value), argc);
            }
    
            runtimeError(vm, "Set has no function %s().", name->str);
            return false;
        }
        case OBJ_ABSTRACT: {
            ObjAbstract *abstract = AS_ABSTRACT(receiver);
            Value value;
            if (tableGet(&abstract->values, name, &value)) {
                return callNativeFunction(vm, AS_NATIVE(value), argc);
            }
    
            runtimeError(vm, "Object has no function '%s'.", name->str);
            return false;
        }
        default: break;
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
    ObjFunction *function = AS_CLOSURE(method)->function;

    if (function->accessLevel == ACCESS_PRIVATE) {
        tableSet(vm, &objClass->privateMethods, name, method, ILEX_READ_ONLY);
    } else {
        if (function->type == TYPE_ABSTRACT) {
            tableSet(vm, &objClass->abstractMethods, name, method, ILEX_READ_ONLY);
        } else {
            tableSet(vm, &objClass->methods, name, method, ILEX_READ_ONLY);
        }
    }

    pop(vm);
}

static void createClass(VM *vm, ObjString *name, ObjClass *superClass, ClassType type) {
    ObjClass *objClass = newClass(vm, name, superClass, type);
    push(vm, OBJ_VAL(objClass));

    if (superClass != NULL) {
        tableAddAll(vm, &superClass->methods, &objClass->methods);
        tableAddAll(vm, &superClass->abstractMethods, &objClass->abstractMethods);
    
        tableAddAll(vm, &superClass->fields, &objClass->fields);
        tableAddAll(vm, &superClass->privateFields, &objClass->privateFields);
    }
}

void registerBaseClass(VM *vm, const char *name) {
    createClass(vm, copyString(vm, name, (int)strlen(name)), NULL, CLASS_ABSTRACT);
}

void registerClassFunction(VM *vm, ObjClass *objClass, const char *name, NativeFn function, AccessLevel accessLevel) {

}
void registerClassVariable(VM *vm, ObjClass *objClass, const char *name, NativeFn function, bool isPrivate);
void registerClassStaticVariable(VM *vm, ObjClass *objClass, const char *name, NativeFn function, bool isConst);

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

InterpretResult run(VM *vm, int frameIndex, Value *val) {
    CallFrame *frame = &vm->frames[vm->frameCount - 1];
    // printf("frameCount %d\n", vm->frameCount);
    register uint8_t *ip = frame->ip;

#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_SHORT()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueTypeArg, op, type) \
    do { \
      if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) { \
        frame->ip = ip; \
        runtimeError(vm, "Operands must be numbers. Got '%s', '%s' (%s, %s).", valueType(peek(vm, 0)), valueType(peek(vm, 1)), valueToString(peek(vm, 0)), valueToString(peek(vm, 1))); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      type b = AS_NUMBER(pop(vm)); \
      type a = AS_NUMBER(pop(vm)); \
      push(vm, valueTypeArg(a op b)); \
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
        disassembleInstruction(&frame->closure->function->chunk, (int)(ip - frame->closure->function->chunk.code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(vm, constant);
            } break;
            case OP_NULL:  push(vm, NULL_VAL); break;
            case OP_EMPTY: push(vm, ERROR_VAL); break; // Does nothing.
            case OP_TRUE:  push(vm, BOOL_VAL(true)); break;
            case OP_FALSE: push(vm, BOOL_VAL(false)); break;
            case OP_POP: pop(vm); break;
            case OP_GET_LOCAL: {
                uint16_t slot = READ_SHORT();
                push(vm, frame->slots[slot]);
            } break;
            case OP_GET_GLOBAL: {
                ObjString *name = READ_STRING();
                Value value;
                if (!tableGet(&vm->globals, name, &value)) {
                    frame->ip = ip;
                    runtimeError(vm, "GET_GLOBAL: Undefined variable '%s'.", name->str);

                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, value);
            } break;
            case OP_GET_SCRIPT: {
                ObjString *name = READ_STRING();
                Value value;
                if (!tableGet(&frame->closure->function->script->values, name, &value)) {
                    frame->ip = ip;
                    runtimeError(vm, "GET_SCRIPT: Undefined variable '%s'.", name->str);
    
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, value);
            } break;
            case OP_GET_UPVALUE: {
                uint16_t slot = READ_SHORT();
                push(vm, *frame->closure->upvalues[slot]->location);
            } break;
            case OP_GET_PROPERTY: {
                Value receiver = peek(vm, 0);
                if (!IS_OBJ(receiver)) {
                    char *type = valueType(receiver);
                    frame->ip = ip;
                    runtimeError(vm, "Type '%s' has no properties.", type);
                    free(type);
                    return INTERPRET_RUNTIME_ERROR;
                }

                switch ((getObjType(receiver))) {
                    case OBJ_INSTANCE: {
                        ObjInstance *instance = AS_INSTANCE(receiver);
                        ObjString *name = READ_STRING();

                        Value value;
                        if (tableGet(&instance->fields, name, &value)) {
                            pop(vm); // Instance.
                            push(vm, value);
                            break;
                        }
                        if (tableGet(&instance->privateFields, name, &value)) {
                            frame->ip = ip;
                            runtimeError(vm, "Can't access private property '%s' on '%s' instance.", name->str, instance->objClass->name->str);
                            return INTERPRET_RUNTIME_ERROR;
                        }
                        
                        if (bindMethod(vm, instance->objClass, name)) {
                            break;
                        }
    
                        frame->ip = ip;
                        runtimeError(vm, "'%s' instance does not have property: '%s'.", instance->objClass->name->str, name->str);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    case OBJ_ENUM: {
                        ObjEnum *enumObj = AS_ENUM(receiver);
                        ObjString *name = READ_STRING();
                        Value value;

                        if (tableGet(&enumObj->values, name, &value)) {
                            pop(vm); // Enum.
                            push(vm, value);
                            break;
                        }
    
                        frame->ip = ip;
                        runtimeError(vm, "'%s' enum does not have property: '%s'.", enumObj->name->str, name->str);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    case OBJ_SCRIPT: {
                        ObjScript *script  = AS_SCRIPT(receiver);
                        ObjString *name = READ_STRING();
                        Value value;
                        if (tableGet(&script->values, name, &value)) {
                            pop(vm); // Script.
                            push(vm, value);
                            break;
                        }
    
                        frame->ip = ip;
                        runtimeError(vm, "'%s' does not have property: '%s'.", script->name->str, name->str);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    case OBJ_CLASS: {
                        ObjClass *objClass = AS_CLASS(receiver);
                        ObjClass *checkingClass = objClass;
                        ObjString *name = READ_STRING();
                        
                        bool found = false;
                        Value value;
                        while (objClass != NULL) {
                            if (tableGet(&objClass->staticConsts, name, &value)) {
                                pop(vm); // Class.
                                push(vm, value);
                                found = true;
                                break;
                            }
                            
                            if (tableGet(&objClass->staticVars, name, &value)) {
                                pop(vm);
                                push(vm, value);
                                found = true;
                                break;
                            }
                            
                            objClass = objClass->superClass;
                        }
    
                        if (!found) {
                            frame->ip = ip;
                            runtimeError(vm, "'%s' does not have property '%s'.", checkingClass->name->str, name->str);
                            return INTERPRET_RUNTIME_ERROR;
                        }
                    } break;
                    case OBJ_MAP: {
                        ObjMap *map = AS_MAP(receiver);
                        Value name = READ_CONSTANT();

                        pop(vm); // map

                        Value v;
                        if (mapGet(map, name, &v)) {
                            push(vm, v);
                        } else {
                            push(vm, NULL_VAL);
                        }
                    } break;
                    default: {
                        char *type = valueType(receiver);
                        frame->ip = ip;
                        runtimeError(vm, "Type '%s' has no properties.", type);
                        free(type);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
            } break;
            case OP_GET_PROPERTY_NO_POP: {
                if (!IS_INSTANCE(peek(vm, 0))) {
                    frame->ip = ip;
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
    
                if (bindMethod(vm, instance->objClass, name)) {
                    break;
                }

                if (tableGet(&instance->privateFields, name, &value)) {
                    frame->ip = ip;
                    runtimeError(vm, "Can't access private property '%s' on '%s' instance.", name->str, instance->objClass->name->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
    
                frame->ip = ip;
                runtimeError(vm, "'%s' instance does not have property: '%s'.", instance->objClass->name->str, name->str);
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_GET_PRIVATE_PROPERTY: {
                if (!IS_INSTANCE(peek(vm, 0))) {
                    frame->ip = ip;
                    runtimeError(vm, "Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(vm, 0));
                ObjString *name = READ_STRING();
                Value value;

                if (tableGet(&instance->privateFields, name, &value)) {
                    pop(vm); // Instance.
                    push(vm, value);
                    break;
                }

                if (tableGet(&instance->fields, name, &value)) {
                    pop(vm); // Instance.
                    push(vm, value);
                    break;
                }

                if (bindMethod(vm, instance->objClass, name)) {
                    break;
                }
    
                frame->ip = ip;
                runtimeError(vm, "'%s' instance does not have property: '%s'.", instance->objClass->name->str, name->str);
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_GET_PRIVATE_PROPERTY_NO_POP: {
                if (!IS_INSTANCE(peek(vm, 0))) {
                    frame->ip = ip;
                    runtimeError(vm, "Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(vm, 0));
                ObjString *name = READ_STRING();
                Value value;

                if (tableGet(&instance->privateFields, name, &value)) {
                    push(vm, value);
                    break;
                }

                if (tableGet(&instance->fields, name, &value)) {
                    push(vm, value);
                    break;
                }

                if (bindMethod(vm, instance->objClass, name)) {
                    break;
                }
    
                frame->ip = ip;
                runtimeError(vm, "'%s' instance does not have property: '%s'.", instance->objClass->name->str, name->str);
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_GET_SUPER: {
                ObjString *name = READ_STRING();
                ObjClass *superclass = AS_CLASS(pop(vm));

                if (!bindMethod(vm, superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_DEFINE_GLOBAL: {
                ObjString *name = READ_STRING();
                tableSet(vm, &vm->globals, name, peek(vm, 0), ILEX_READ_WRITE);
                pop(vm);
            } break;
            case OP_DEFINE_SCRIPT: {
                ObjString *name = READ_STRING();
                tableSet(vm, &frame->closure->function->script->values, name, peek(vm, 0), ILEX_READ_WRITE);
                pop(vm);
            } break;
            case OP_SET_LOCAL: {
                uint16_t slot = READ_SHORT();
                frame->slots[slot] = peek(vm, 0);
            } break;
            case OP_SET_GLOBAL: {
                ObjString *name = READ_STRING();
                if (tableSet(vm, &vm->globals, name, peek(vm, 0), ILEX_READ_WRITE)) {
                    tableDelete(&vm->globals, name);
                    frame->ip = ip;
                    runtimeError(vm, "SET_GLOBAL: Undefined variable '%s'.", name->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_SET_SCRIPT: {
                ObjString *name = READ_STRING();
                if (tableSet(vm, &frame->closure->function->script->values, name, peek(vm, 0), ILEX_READ_WRITE)) {
                    tableDelete(&frame->closure->function->script->values, name);
                    frame->ip = ip;
                    runtimeError(vm, "SET_SCRIPT: Undefined variable '%s'.", name->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_SET_UPVALUE: {
                uint16_t slot = READ_SHORT();
                *frame->closure->upvalues[slot]->location = peek(vm, 0);
            } break;
            case OP_SET_PROPERTY: {
                if (IS_SCRIPT(peek(vm, 1))) {
                    ObjScript *script = AS_SCRIPT(peek(vm, 1));
                    ObjString *name = READ_STRING();
                    Value unused;
                    
                    if (tableGet(&vm->consts, name, &unused)) {
                        frame->ip = ip;
                        runtimeError(vm, "Cannot assign to const variable '%s'.", name->str);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    
                    tableSet(vm, &script->values, name, peek(vm, 0), ILEX_READ_WRITE);
                    Value value = pop(vm);
                    pop(vm); // Script.
                    push(vm, value);
                } else if (IS_INSTANCE(peek(vm, 1))) {
                    ObjInstance *instance = AS_INSTANCE(peek(vm, 1));
                    ObjString *var = READ_STRING();
                    
                    // TODO: Move these checks to the compiler. Have an instance map.
                    Value unused;
                    if (tableGet(&instance->privateFields, var, &unused)) {
                        frame->ip = ip;
                        runtimeError(vm, "Cannot assign to private variable '%s'.", var->str);
                        return INTERPRET_RUNTIME_ERROR;
                    }

                    if (!tableGet(&instance->fields, var, &unused)) {
                        frame->ip = ip;
                        runtimeError(vm, "Instance of '%s' contains no variable '%s'.", instance->objClass->name->str, var->str);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    
                    tableSet(vm, &instance->fields, var, peek(vm, 0), ILEX_READ_WRITE);
                    Value value = pop(vm);
                    pop(vm); // Instance.
                    push(vm, value);
                } else if (IS_CLASS(peek(vm, 1))) {
                    ObjClass *objClass = AS_CLASS(peek(vm, 1));
                    ObjString *var = READ_STRING();
                    
                    // TODO: Move these check to the compiler.
                    Value unused;
                    if (tableGet(&objClass->staticConsts, var, &unused)) {
                        frame->ip = ip;
                        runtimeError(vm, "Cannot assign to a class constant '%s'.", var->str);
                        return INTERPRET_RUNTIME_ERROR;
                    } /*else if (!tableGet(&objClass->fields, var, &unused)) {
                        frame->ip = ip;
                        runtimeError(vm, "Class '%s' contains no static variable '%s'.", objClass->name->str, var->str);
                        return INTERPRET_RUNTIME_ERROR;
                    } */
                    
                    // If it is static set that.
                    // TODO: Have the compiler generate OP_SET_CLASS_STATIC_VAR for Class.staticVar = thing
                    
                    if (tableGet(&objClass->staticVars, var, &unused)) {
                        tableSet(vm, &objClass->staticVars, var, peek(vm, 0), ILEX_READ_WRITE);
                    } else {
                        tableSet(vm, &objClass->fields, var, peek(vm, 0), ILEX_READ_WRITE);
                    }
                    pop(vm); // Value.
                    // pop(vm); // Class.
                } else if (IS_MAP(peek(vm, 1))) {
                    ObjMap *map = AS_MAP(peek(vm, 1));
                    Value key = READ_CONSTANT();

                    Value unused;
                    if (!mapGet(map, key, &unused)) {
                        ObjString *str = AS_STRING(key);
                        frame->ip = ip;
                        runtimeError(vm, "Key '%s' not found, cannot add keys to map via the dot operator.", str->str);
                        return INTERPRET_RUNTIME_ERROR;
                    }

                    mapSet(vm, map, key, peek(vm, 0));

                    pop(vm); // map
                } else {
                    char *type = valueType(peek(vm, 1));
                    frame->ip = ip;
                    runtimeError(vm, "Can't set property on type '%s'.", type);
                    free(type);
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_SET_PRIVATE_PROPERTY: {
                if (IS_INSTANCE(peek(vm, 1))) {
                    ObjInstance *instance = AS_INSTANCE(peek(vm, 1));
                    tableSet(vm, &instance->privateFields, READ_STRING(), peek(vm, 0), ILEX_READ_WRITE);
                    pop(vm);
                    pop(vm);
                    push(vm, NULL_VAL);
                } else if (IS_CLASS(peek(vm, 1))) {
                    ObjClass *objClass = AS_CLASS(peek(vm, 1));
                    tableSet(vm, &objClass->privateFields, READ_STRING(), peek(vm, 0), ILEX_READ_WRITE);
                    pop(vm); // Value.
//                    pop(vm); // Class.
                }
            } break;
            case OP_SET_CLASS_STATIC_VAR: {
                ObjClass *objClass = AS_CLASS(peek(vm, 1));
                ObjString *key = READ_STRING();
                bool isConst = READ_BYTE();

                if (isConst) {
                    tableSet(vm, &objClass->staticConsts, key, peek(vm, 0), ILEX_READ_ONLY);
                } else {
                    tableSet(vm, &objClass->staticVars, key, peek(vm, 0), ILEX_READ_WRITE);
                }

                pop(vm);
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
            case OP_GR: BINARY_OP(BOOL_VAL, >, double); break;
            case OP_GREQ: BINARY_OP(BOOL_VAL, >=, double); break;
            case OP_LT: BINARY_OP(BOOL_VAL, <, double); break;
            case OP_LTEQ: BINARY_OP(BOOL_VAL, <=, double); break;
            case OP_ADD: {
                if (IS_NUMBER(peek(vm, 0)) && IS_NUMBER(peek(vm, 1))) {
                    double b = AS_NUMBER(pop(vm));
                    double a = AS_NUMBER(pop(vm));
                    push(vm, NUMBER_VAL(a + b));
                } else if (IS_STRING(peek(vm, 0)) && IS_STRING(peek(vm, 1))) {
                    concat(vm);
                } else {
                    frame->ip = ip;
                    runtimeError(vm, "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_CONCAT: {
                if (!IS_STRING(peek(vm, 0)) || !IS_STRING(peek(vm, 1))) {
                    frame->ip = ip;
                    runtimeError(vm, "Concat operands must be two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                concat(vm);
            } break;
            case OP_INC: {
                if (!IS_NUMBER(peek(vm, 0))) {
                    frame->ip = ip;
                    runtimeError(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                push(vm, NUMBER_VAL(AS_NUMBER(pop(vm)) + 1));
            } break;
            case OP_SUB: BINARY_OP(NUMBER_VAL, -, double); break;
            case OP_DEC: {
                if (!IS_NUMBER(peek(vm, 0))) {
                    frame->ip = ip;
                    runtimeError(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
        
                push(vm, NUMBER_VAL(AS_NUMBER(pop(vm)) - 1));
            } break;
            case OP_MUL: BINARY_OP(NUMBER_VAL, *, double); break;
            case OP_DIV: BINARY_OP(NUMBER_VAL, /, double); break;
            case OP_POW: {
                if (!IS_NUMBER(peek(vm, 0) || !IS_NUMBER(peek(vm, 1)))) {
                    frame->ip = ip;
                    runtimeError(vm, "Operands must be two numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop(vm));
                double a = AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL(powf(a, b)));
            } break;
            case OP_MOD: {
                if (!IS_NUMBER(peek(vm, 0) || !IS_NUMBER(peek(vm, 1)))) {
                    frame->ip = ip;
                    runtimeError(vm, "Operands must be two numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop(vm));
                double a = AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL(fmod(a, b)));
            } break;
            case OP_BIT_AND: BINARY_OP(NUMBER_VAL, &,  int); break;
            case OP_BIT_OR:  BINARY_OP(NUMBER_VAL, |,  int); break;
            case OP_BIT_XOR: BINARY_OP(NUMBER_VAL, ^,  int); break;
            case OP_BIT_LS:  BINARY_OP(NUMBER_VAL, <<, int); break;
            case OP_BIT_RS:  BINARY_OP(NUMBER_VAL, >>, int); break;
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
            case OP_BIT_NOT: {
                if (!IS_NUMBER(peek(vm, 0))) {
                    frame->ip = ip;
                    runtimeError(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, NUMBER_VAL(~(int)AS_NUMBER(pop(vm))));
            } break;
            case OP_NEG: {
                if (!IS_NUMBER(peek(vm, 0))) {
                    frame->ip = ip;
                    runtimeError(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, NUMBER_VAL(-AS_NUMBER(pop(vm))));
            } break;
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                ip += offset;
            } break;
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsy(peek(vm, 0))) {
                    ip += offset;
                }
            } break;
            case OP_JUMP_IF_TRUE: {
                uint16_t offset = READ_SHORT();
                if (!isFalsy(peek(vm, 0))) {
                    ip += offset;
                }
            } break;
            case OP_JUMP_DO_WHILE: {
                uint16_t offset = READ_SHORT();
                if (!isFalsy(peek(vm, 0))) {
                    ip -= offset;
                }
            } break;
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                ip -= offset;
            } break;
            case OP_CALL: {
                int argc = READ_BYTE();
                frame->ip = ip;
                if (!callValue(vm, peek(vm, argc), argc)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
            } break;
            case OP_INVOKE: {
                ObjString *method = READ_STRING();
                int argc = READ_BYTE();
                frame->ip = ip;
                if (!invoke(vm, method, argc)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
            } break;
            case OP_INVOKE_SUPER: {
                ObjString *method = READ_STRING();
                int argc = READ_BYTE();
                frame->ip = ip;
                ObjClass *superclass = AS_CLASS(pop(vm));
                if (!invokeFromClass(vm, superclass, method, argc)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
            } break;
            case OP_INVOKE_THIS: {
                ObjString *method = READ_STRING();
                int argc = READ_BYTE();
                frame->ip = ip;
                if (!invokeFromThis(vm, method, argc)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
            } break;
            case OP_CLOSURE: {
                ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure *closure = newClosure(vm, function);
                push(vm, OBJ_VAL(closure));

                for (int i = 0; i < closure->upvalueCount; ++i) {
                    uint8_t isLocal = READ_BYTE();
                    uint16_t index = READ_SHORT();
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

                // If we are at the frameIndex frame that means we are returning from Ilex code called from c.
                // A frameIndex of -1 indicates that this is running in a normal state.
                if (vm->frameCount == 0 || (frameIndex != -1 && &vm->frames[vm->frameCount - 1] == &vm->frames[frameIndex])) {
                    if (frameIndex != -1) {
                        *val = result;
                        // pop(vm); // TODO: Sometimes this pop is needed, sometimes it causes crashes.
                        // printValueNl(peek(vm, 0));
                        return INTERPRET_GOOD;
                    }

                    pop(vm);
                    return INTERPRET_GOOD;
                }

                vm->stackTop = frame->slots;
                push(vm, result);
                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
            } break;
            case OP_CLASS: {
                ClassType type = READ_BYTE();
                createClass(vm, READ_STRING(), NULL, type);
            } break;
            case OP_INHERIT: {
                ClassType type = READ_BYTE();
                Value superClass = peek(vm, 0);
                if (!IS_CLASS(superClass)) {
                    char *vt = valueType(superClass);
                    frame->ip = ip;
                    runtimeError(vm, "Superclass must be a class, got a '%s' instead.", vt);
                    free(vt);
                    return INTERPRET_RUNTIME_ERROR;
                }
                createClass(vm, READ_STRING(), AS_CLASS(superClass), type);
            } break;
            // TODO: Can this be moved to the compiler?
            case OP_CHECK_ABSTRACT: {
                ObjClass *objClass = AS_CLASS(peek(vm, 0));

                for ( int i = 0; i < objClass->abstractMethods.capacity; ++i) {
                    ObjString *key = objClass->abstractMethods.entries[i].key;
                    if (key == NULL) {
                        continue;
                    }

                    Value unused;
                    if (!tableGet(&objClass->methods, key, &unused)) {
                        frame->ip = ip;
                        runtimeError(vm, "Class '%s' doesn't implement abstract method '%s'.", objClass->name->str, objClass->abstractMethods.entries[i].key->str);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
            } break;
            case OP_METHOD: {
                defineMethod(vm, READ_STRING());
            }break;
            case OP_ASSERT: {
                Value condition = pop(vm);
                ObjString *error = READ_STRING();

                if (isFalsy(condition)) {
                    if (!error->str[0]) {
                        assertError(vm, "\033[31mAssertion Failed with no message.\033[m");
                    } else {
                        assertError(vm, "\033[31mAssertion failed\033[m with message: %s", error->str);
                    }
                    
                    return INTERPRET_ASSERT_ERROR;
                }
            } break;
            case OP_PANIC: {
                ObjString *error = READ_STRING();
                panicError(vm, error->str);
                return INTERPRET_PANIC_ERROR;
            }
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
                if (!vm->fallThrough && !valuesEqual(peek(vm,0), a)) {
                    ip += offset;
                } else {
                    pop(vm); // switch expression.
                    vm->fallThrough = false;
                }
            } break;
            case OP_CMP_JMP_FALL: {
                uint16_t offset = READ_SHORT();
                Value a = pop(vm);
                if (!vm->fallThrough && !valuesEqual(peek(vm,0), a)) {
                    ip += offset;
                } else {
                    pop(vm); // switch expression.
                    vm->fallThrough = true;
                }
            } break;
            case OP_ENUM: {
                ObjEnum *enumObj = newEnum(vm, READ_STRING());
                push(vm, OBJ_VAL(enumObj));
            } break;
            case OP_ENUM_SET_VALUE: {
                Value value = peek(vm, 0);
                ObjEnum *enumObj = AS_ENUM(peek(vm, 1));

                tableSet(vm, &enumObj->values, READ_STRING(), value, ILEX_READ_ONLY);
                pop(vm);
            } break;
            case OP_USE: {
                ObjString *filename = READ_STRING();
                Value scriptVal;
                
                if (tableGet(&vm->scripts, filename, &scriptVal)) {
                    vm->lastScript = AS_SCRIPT(scriptVal);
                    push(vm, NULL_VAL);
                    break;
                }

                char filenameStr[1024];
                memcpy(filenameStr, filename->str, filename->len);
                filenameStr[filename->len] = '\0';
                size_t len = strlen(filenameStr);

                if (strcmp(filenameStr + len - 5, ".ilex") != 0) {
                    strncat(filenameStr, ".ilex", 5);
                    len += 5;
                }

                char path[I_MAX_PATH];
                if (!resolvePath(frame->closure->function->script->path->str, filenameStr, path)) {
                    frame->ip = ip;
                    runtimeError(vm, "Coule not open file '%s'.", filenameStr);
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                char *src = readFile(path);
                if (src == NULL) {
                    frame->ip = ip;
                    runtimeError(vm, "Could not open file '%s'.", filenameStr);
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                ObjString *pathStr = copyString(vm, path, (int)len);
                push(vm, OBJ_VAL(pathStr));
                ObjScript *script = newScript(vm, pathStr);
                script->path = dirName(vm, path, len);
                vm->lastScript = script;
                pop(vm);
                
                push(vm, OBJ_VAL(script));
                ObjFunction *function = compile(vm, script, src);
                pop(vm);
    
                FREE_ARRAY(vm, char, src, strlen(src));
                
                if (function == NULL) {
                    return INTERPRET_COMPILE_ERROR;
                }
                
                push(vm, OBJ_VAL(function));
                ObjClosure *closure = newClosure(vm, function);
                pop(vm);
                push (vm, OBJ_VAL(closure));
                
                frame->ip = ip;
                call(vm, closure, 0);
                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
            } break;
            case OP_USE_VAR: {
                push(vm, OBJ_VAL(vm->lastScript));
            } break;
            case OP_USE_BUILTIN: {
                int idx = READ_BYTE();
                ObjString *fileName = READ_STRING();
                Value libVal;

                // Skip if used already.
                if (tableGet(&vm->scripts, fileName, &libVal)) {
                    // vm->lastScript = AS_SCRIPT(libVal);
                    push(vm, libVal);
                    break;
                }

                Value lib = useBuiltInLib(vm, idx);

                if (IS_ERR(lib)) {
                    return INTERPRET_COMPILE_ERROR;
                }

                push(vm, lib);
            } break;
            case OP_USE_BUILTIN_VAR: {
                ObjString *fileName = READ_STRING();
                int varCount = READ_BYTE();

                Value libVal;
                ObjScript *script;

                if (tableGet(&vm->scripts, fileName, &libVal)) {
                    script = AS_SCRIPT(libVal);
                } else {
                    frame->ip = ip;
                    runtimeError(vm, "Unknown error.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                for (int i = 0; i < varCount; i++) {
                    Value libVar;
                    ObjString *variable = READ_STRING();

                    if (!tableGet(&script->values, variable, &libVar)) {
                        frame->ip = ip;
                        runtimeError(vm, "'%s' can't be found in library '%s'.", variable->str, script->name->str);
                        return INTERPRET_RUNTIME_ERROR;
                    }

                    push(vm, libVar);
                }
            } break;
            case OP_USE_END:
                vm->lastScript = frame->closure->function->script;
                break;
            case OP_BREAK: break; // lol
            case OP_NEW_ARRAY: {
                int count = READ_BYTE();
                ObjArray *array = newArray(vm);
                push(vm, OBJ_VAL(array));
                
                for (int i = count; i > 0; --i) {
                    writeValueArray(vm, &array->data, peek(vm, i));
                }

#ifdef DEBUG_MODE
                vm->stackHeight -= count + 1;
#endif
                vm->stackTop -= count + 1;
                push(vm, OBJ_VAL(array));
            } break;
            case OP_INDEX: {
                Value indexValue = peek(vm, 0);
                Value receiver = peek(vm, 1);
    
                if (!IS_OBJ(receiver)) {
                    char *type = valueType(receiver);
                    frame->ip = ip;
                    runtimeError(vm, "Type '%s' is not indexable.", type);
                    free(type);
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                switch (getObjType(receiver)) {
                    case OBJ_ARRAY: {
                        if (!IS_NUMBER(indexValue)) {
                            frame->ip = ip;
                            runtimeError(vm, "Array index must be a number.");
                            return INTERPRET_RUNTIME_ERROR;
                        }
        
                        ObjArray *array = AS_ARRAY(receiver);
                        int idx = AS_NUMBER(indexValue);
                        int oIdx = idx;
        
                        if (idx < 0) {
                            idx = array->data.count + idx;
                        }
        
                        if (idx >= 0 && idx < array->data.count) {
                            pop(vm);
                            pop(vm);
                            push(vm, array->data.values[idx]);
                            break;
                        }
    
                        frame->ip = ip;
                        runtimeError(vm, "Array index '%d' out of bounds.", oIdx);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    case OBJ_STRING: {
                        if (!IS_NUMBER(indexValue)) {
                            frame->ip = ip;
                            runtimeError(vm, "Array index must be a number.");
                            return INTERPRET_RUNTIME_ERROR;
                        }
        
                        ObjString *str = AS_STRING(receiver);
                        int idx = AS_NUMBER(indexValue);
                        int oIdx = idx;
        
                        if (idx < 0) {
                            idx = str->len + idx;
                        }
        
                        if (idx >= 0 && idx < str->len) {
                            pop(vm);
                            pop(vm);
                            push(vm, OBJ_VAL(copyString(vm, &str->str[idx], 1)));
                            break;
                        }
    
                        frame->ip = ip;
                        runtimeError(vm, "String index '%d' out of bounds.", oIdx);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    case OBJ_MAP: {
                        ObjMap *map = AS_MAP(receiver);
                        if (!isValidKey(indexValue)) {
                            char *type = valueType(indexValue);
                            frame->ip = ip;
                            runtimeError(vm, "Expect string or number for key but got '%s'.", type);
                            free(type);
                            return INTERPRET_RUNTIME_ERROR;
                        }
                        
                        Value v;
                        pop(vm);
                        pop(vm);
                        
                        if (mapGet(map, indexValue, &v)) {
                            push(vm, v);
                        } else {
                            push(vm, NULL_VAL); // Return null if the key doesn't exist.
                        }
                    } break;
                    default: {
                        char *type = valueType(receiver);
                        frame->ip = ip;
                        runtimeError(vm, "Type '%s' is not indexable.", type);
                        free(type);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
            } break;
            case OP_INDEX_ASSIGN: {
                Value assignValue = peek(vm, 0);
                Value indexValue = peek(vm, 1);
                Value receiver = peek(vm, 2);
    
                if (!IS_OBJ(receiver)) {
                    char *type = valueType(receiver);
                    frame->ip = ip;
                    runtimeError(vm, "Type '%s' is not indexable.", type);
                    free(type);
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                switch (getObjType(receiver)) {
                    case OBJ_ARRAY: {
                        if (!IS_NUMBER(indexValue)) {
                            frame->ip = ip;
                            runtimeError(vm, "Array index must be a number.");
                            return INTERPRET_RUNTIME_ERROR;
                        }
    
                        ObjArray *array = AS_ARRAY(receiver);
                        int idx = AS_NUMBER(indexValue);
                        int oIdx = idx;
    
                        if (idx < 0) {
                            idx = array->data.count + idx;
                        }
    
                        if (idx >= 0 && idx < array->data.count) {
                            array->data.values[idx] = assignValue;
                            pop(vm);
                            pop(vm);
                            pop(vm);
                            push(vm, NULL_VAL);
                            break;
                        }
    
                        frame->ip = ip;
                        runtimeError(vm, "Array index '%d' out of bounds.", oIdx);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    case OBJ_STRING: {
                        if (!IS_NUMBER(indexValue)) {
                            frame->ip = ip;
                            runtimeError(vm, "Array index must be a number.");
                            return INTERPRET_RUNTIME_ERROR;
                        }
                        
                        if (!IS_STRING(assignValue)) {
                            frame->ip = ip;
                            runtimeError(vm, "Assign value must be a string.");
                            return INTERPRET_RUNTIME_ERROR;
                        }
        
                        ObjString *str = AS_STRING(receiver);
                        ObjString *assignStr = AS_STRING(assignValue);
                        int idx = AS_NUMBER(indexValue);
                        int oIdx = idx;
        
                        if (idx < 0) {
                            idx = str->len + idx;
                        }
        
                        if (idx >= 0 && idx < str->len) {
                            str->str[idx] = assignStr->str[0];
                            pop(vm);
                            pop(vm);
                            pop(vm);
                            push(vm, NULL_VAL);
                            break;
                        }
    
                        frame->ip = ip;
                        runtimeError(vm, "String index '%d' out of bounds.", oIdx);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    case OBJ_MAP: {
                        ObjMap *map = AS_MAP(receiver);
                        if (!isValidKey(indexValue)) {
                            char *type = valueType(indexValue);
                            frame->ip = ip;
                            runtimeError(vm, "Expect string or number for key but got '%s'.", type);
                            free(type);
                            return INTERPRET_RUNTIME_ERROR;
                        }
    
                        mapSet(vm, map, indexValue, assignValue);
                        pop(vm);
                        pop(vm);
                        pop(vm);
                        push(vm, NULL_VAL);
                    } break;
                    default: {
                        char *type = valueType(receiver);
                        frame->ip = ip;
                        runtimeError(vm, "Type '%s' is not indexable.", type);
                        free(type);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
            } break;
            case OP_INDEX_PUSH: {
                Value pushValue = peek(vm, 0);
                Value indexValue = peek(vm, 1);
                Value receiver = peek(vm, 2);
    
                if (!IS_OBJ(receiver)) {
                    char *type = valueType(receiver);
                    frame->ip = ip;
                    runtimeError(vm, "Type '%s' is not indexable.", type);
                    free(type);
                    return INTERPRET_RUNTIME_ERROR;
                }
    
                switch (getObjType(receiver)) {
                    case OBJ_ARRAY: {
                        if (!IS_NUMBER(indexValue)) {
                            frame->ip = ip;
                            runtimeError(vm, "Array index must be a number.");
                            return INTERPRET_RUNTIME_ERROR;
                        }
            
                        ObjArray *array = AS_ARRAY(receiver);
                        int idx = AS_NUMBER(indexValue);
                        int oIdx = idx;
            
                        if (idx < 0) {
                            idx = array->data.count + idx;
                        }
            
                        if (idx >= 0 && idx < array->data.count) {
                            vm->stackTop[-1] = array->data.values[idx];
                            push(vm, pushValue);
                            break;
                        }
    
                        frame->ip = ip;
                        runtimeError(vm, "Array index '%d' out of bounds.", oIdx);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    case OBJ_MAP: {
                        ObjMap *map = AS_MAP(receiver);
                        if (!isValidKey(indexValue)) {
                            char *type = valueType(indexValue);
                            frame->ip = ip;
                            runtimeError(vm, "Expect string or number for key but got '%s'.", type);
                            free(type);
                            return INTERPRET_RUNTIME_ERROR;
                        }
                        
                        Value mapValue;
                        if (!mapGet(map, indexValue, &mapValue)) {
                            mapValue = NULL_VAL;
                        }
                        
                        vm->stackTop[-1] = mapValue;
                        push(vm, pushValue);
                    } break;
                    default: {
                        char *type = valueType(receiver);
                        frame->ip = ip;
                        runtimeError(vm, "Type '%s' is not indexable.", type);
                        free(type);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
            } break;
            case OP_SLICE: {
                Value sliceEndIndex = peek(vm, 0);
                Value sliceStartIndex = peek(vm, 1);
                Value receiver = peek(vm, 2);
    
                if (!IS_OBJ(receiver)) {
                    char *type = valueType(receiver);
                    frame->ip = ip;
                    runtimeError(vm, "Type '%s' is not sliceable.", type);
                    free(type);
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                if (!IS_NUMBER(sliceStartIndex) && !IS_ERR(sliceStartIndex)) {
                    frame->ip = ip;
                    runtimeError(vm, "Slice start index must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                if (!IS_NUMBER(sliceEndIndex) && !IS_ERR(sliceEndIndex)) {
                    frame->ip = ip;
                    runtimeError(vm, "Slice end index must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
    
                int indexStart;
                int indexEnd;
                Value returnVal;
                
                if (IS_ERR(sliceStartIndex)) {
                    indexStart = 0;
                } else {
                    indexStart = AS_NUMBER(sliceStartIndex);
    
                    if (indexStart < 0) {
                        indexStart = 0;
                    }
                }
                
                switch (getObjType(receiver)) {
                    case OBJ_ARRAY: {
                        ObjArray *retArray = newArray(vm);
                        push(vm, OBJ_VAL(retArray));
                        ObjArray *array = AS_ARRAY(receiver);
                        
                        if (IS_ERR(sliceEndIndex)) {
                            indexEnd = array->data.count;
                        } else {
                            indexEnd = AS_NUMBER(sliceEndIndex);
                            
                            if (indexEnd > array->data.count) {
                                indexEnd = array->data.count;
                            } else if (indexEnd < 0) {
                                indexEnd = array->data.count + indexEnd; // TODO(Skyler): Fix potential crash here.
                            }
                        }
                        
                        for (int i = indexStart; i < indexEnd; ++i) {
                            writeValueArray(vm, &retArray->data, array->data.values[i]);
                        }
                        
                        pop(vm);
                        returnVal = OBJ_VAL(retArray);
                    } break;
                    case OBJ_STRING: {
                        ObjString *str = AS_STRING(receiver);
    
                        if (IS_ERR(sliceEndIndex)) {
                            indexEnd = str->len;
                        } else {
                            indexEnd = AS_NUMBER(sliceEndIndex);
        
                            if (indexEnd > str->len) {
                                indexEnd = str->len;
                            } else if (indexEnd < 0) {
                                indexEnd = str->len + indexEnd; // TODO(Skyler): Fix potential crash here.
                            }
                        }
                        
                        if (indexStart > indexEnd) {
                            returnVal = OBJ_VAL(copyString(vm, "", 0));
                        } else {
                            returnVal = OBJ_VAL(copyString(vm, str->str + indexStart, indexEnd - indexStart));
                        }
                    } break;
                    default: {
                        char *type = valueType(receiver);
                        frame->ip = ip;
                        runtimeError(vm, "Type '%s' is not sliceable.", type);
                        free(type);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
    
                pop(vm);
                pop(vm);
                pop(vm);
    
                push(vm, returnVal);
            } break;
            case OP_OPEN_FILE: {
                Value flag = peek(vm, 0);
                Value name = peek(vm, 1);
                
                if (!IS_STRING(flag)) {
                    char *type = valueType(flag);
                    frame->ip = ip;
                    runtimeError(vm, "File flag must be a string got '%s'.", type);
                    free(type);
                    return INTERPRET_RUNTIME_ERROR;
                }
    
                if (!IS_STRING(name)) {
                    char *type = valueType(name);
                    frame->ip = ip;
                    runtimeError(vm, "File name must be a string got '%s'.", type);
                    free(type);
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                ObjString *flagStr = AS_STRING(flag);
                ObjString *nameStr = AS_STRING(name);
                
                ObjFile *file = newFile(vm);
#ifdef I_WIN
                errno_t err = fopen_s(&file->file, nameStr->str, flagStr->str);
#else
                file->file = fopen(nameStr->str, flagStr->str);
                errno_t err = file->file == NULL ? -1 : 0;
#endif
                file->path = nameStr->str;
                file->flags = flagStr->str;
                
                if (err != 0) {
                    frame->ip = ip;
                    runtimeError(vm, "Unable to open file '%s'.", file->path);
                    return INTERPRET_RUNTIME_ERROR;
                }
    
                pop(vm);
                pop(vm);
                push(vm, OBJ_VAL(file));
            } break;
            case OP_CLOSE_FILE: {
                uint16_t slot = READ_SHORT();
                Value value = frame->slots[slot];
                ObjFile *file = AS_FILE(value);
                fclose(file->file);
                file->file = NULL;
            } break;
            case OP_NEW_MAP: {
                int count = READ_BYTE();
                ObjMap *map = newMap(vm);
                push(vm, OBJ_VAL(map));
                
                for (int i = count * 2; i > 0; i -= 2) {
                    if (!isValidKey(peek(vm, i))) {
                        char *type = valueType(peek(vm, i));
                        frame->ip = ip;
                        runtimeError(vm, "Expect string or number for key but got '%s'.", type);
                        free(type);
                        return INTERPRET_RUNTIME_ERROR;
                    }
    
                    mapSet(vm, map, peek(vm, i), peek(vm, i - 1));
                }

#ifdef DEBUG_MODE
                vm->stackHeight -= count * 2 + 1;
#endif
                vm->stackTop -= count * 2 + 1;
                push(vm, OBJ_VAL(map));
            } break;
            case OP_NEW_SET: {
                int count = READ_BYTE();
                ObjSet *set = newSet(vm);
                push(vm, OBJ_VAL(set));
                
                for (int i = count; i > 0; --i) {
                    if (!isValidKey(peek(vm, i))) {
                        char *type = valueType(peek(vm, i));
                        frame->ip = ip;
                        runtimeError(vm, "Expect string or number for value but got '%s'.", type);
                        free(type);
                        return INTERPRET_RUNTIME_ERROR;
                    }
    
                    setAdd(vm, set, peek(vm, i));
                }

#ifdef DEBUG_MODE
                vm->stackHeight -= count + 1;
#endif
                vm->stackTop -= count + 1;
                push(vm, OBJ_VAL(set));
            } break;
            case OP_DEFINE_DEFAULT: {
                int arity = READ_BYTE();
                int arityDefault = READ_BYTE();
                int argc = (int)(vm->stackTop - frame->slots - arityDefault - 1);
                
                Value values[255];
                int index;
                
                for (index = 0; index < arityDefault + argc; ++index) {
                    values[index] = pop(vm);
                }
                
                --index;
                
                for (int i = 0; i < argc; ++i) {
                    push(vm, values[index - i]);
                }
                
                // How many default values are required.
                int remaining = arity + arityDefault - argc;
                
                // Push any default values back on to the stack.
                for (int i = remaining; i > 0; --i) {
                    push(vm, values[i - 1]);
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

InterpretResult interpret(VM *vm, const char *scriptName, const char *source) {
    ObjString *name = copyString(vm, scriptName, (int)strlen(scriptName));
    push(vm, OBJ_VAL(name));
    ObjScript *script = newScript(vm, name);
    pop(vm);
    
    push(vm, OBJ_VAL(script));
    script->path = getDir(vm, scriptName);
    pop(vm);
    
    ObjFunction *function = compile(vm, script, source);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    push(vm, OBJ_VAL(function));
    ObjClosure *closure = newClosure(vm, function);
    pop(vm);
    push(vm, OBJ_VAL(closure));
    call(vm, closure, 0);

    return run(vm, -1, NULL);
}

void runFile(VM *vm, const char *path) {
    char *source = readFile(path);
    InterpretResult res = interpret(vm, path, source);
    free(source);
    
    switch (res) {
        case INTERPRET_COMPILE_ERROR: exit(99);
        case INTERPRET_RUNTIME_ERROR: exit(114);
        case INTERPRET_ASSERT_ERROR:  exit(97);
        case INTERPRET_PANIC_ERROR:   exit(112);
        case INTERPRET_GOOD:
        default: break;
    }
}
