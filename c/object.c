//
// Created by Skyler on 3/13/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(vm, type, objectType) (type*)allocateObject(vm, sizeof(type), objectType)

static Obj* allocateObject(VM *vm, size_t size, ObjType type) {
    Obj *object = (Obj*)reallocate(vm, NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    object->next = vm->objects;
    vm->objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

    return object;
}

ObjBoundMethod *newBoundMethod(VM *vm, Value receiver, ObjClosure *method) {
    ObjBoundMethod *bound = ALLOCATE_OBJ(vm, ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClass *newClass(VM *vm, ObjString *name) {
    ObjClass* objClass = ALLOCATE_OBJ(vm, ObjClass, OBJ_CLASS);
    objClass->name = name;
    initTable(&objClass->methods);

    return objClass;
}

ObjClosure *newClosure(VM *vm, ObjFunction *function) {
    ObjUpvalue **upvalues = ALLOCATE(vm, ObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; ++i) {
        upvalues[i] = NULL;
    }

    ObjClosure *closure = ALLOCATE_OBJ(vm, ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;

    return closure;
}

ObjFunction *newFunction(VM *vm) {
    ObjFunction* function = ALLOCATE_OBJ(vm, ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);

    return function;
}

ObjInstance *newInstance(VM *vm, ObjClass *objClass) {
    ObjInstance *instance = ALLOCATE_OBJ(vm, ObjInstance, OBJ_INSTANCE);
    instance->objClass = objClass;
    initTable(&instance->fields);

    return instance;
}

ObjScript *newScript(VM *vm, ObjString* name) {
    Value moduleVal;
    if (tableGet(&vm->scripts, name, &moduleVal)) {
        return AS_SCRIPT(moduleVal);
    }

    ObjScript *script = ALLOCATE_OBJ(vm, ObjScript, OBJ_SCRIPT);
    initTable(&script->values);
    script->name = name;
    script->path = NULL;

    push(vm, OBJ_VAL(script));
    ObjString *file = copyString(vm, "$file", 5);
    push(vm, OBJ_VAL(file));

    tableSet(vm, &script->values, file, OBJ_VAL(name));
    tableSet(vm, &vm->scripts, name, OBJ_VAL(script));

    pop(vm);
    pop(vm);

    return script;
}

ObjNative *newNative(VM *vm, NativeFn function) {
    ObjNative* native = ALLOCATE_OBJ(vm, ObjNative, OBJ_NATIVE);
    native->function = function;

    return native;
}

static ObjString *allocateString(VM *vm, char *str, int len, uint32_t hash) {
    ObjString *string = ALLOCATE_OBJ(vm, ObjString, OBJ_STRING);
    string->len = len;
    string->str = str;
    string->hash = hash;

    push(vm, OBJ_VAL(string));
    tableSet(vm, &vm->strings, string, NULL_VAL);
    pop(vm);

    return string;
}

static uint32_t hashString(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }

    return hash;
}

char *newCString(const char *str) {
    size_t len = strlen(str);

    char* ret = (char*)malloc(sizeof(char) * (len + 1));
    memcpy(ret, str, len);
    ret[len] = '\0';

    return ret;
}

char *newCStringLen(const char *str, int len) {
    char* ret = (char*)malloc(sizeof(char) * (len + 1));
    memcpy(ret, str, len);
    ret[len] = '\0';

    return ret;
}

ObjString *takeString(VM *vm, char *str, int len) {
    uint32_t hash = hashString(str, len);
    ObjString *interned = tableFindString(&vm->strings, str, len, hash);
    if (interned != NULL) {
        FREE_ARRAY(vm, char, str, len + 1);
        return interned;
    }
    return allocateString(vm, str, len, hash);
}

ObjString *copyString(VM *vm, const char *str, int len) {
    uint32_t hash = hashString(str, len);
    ObjString *interned = tableFindString(&vm->strings, str, len, hash);
    if (interned != NULL) {
        return interned;
    }
    char *heapStr = ALLOCATE(vm, char, len + 1);
    memcpy(heapStr, str, len);
    heapStr[len] = '\0';

    return allocateString(vm, heapStr, len, hash);
}

ObjUpvalue *newUpvalue(VM *vm, Value *slot) {
    ObjUpvalue *upvalue = ALLOCATE_OBJ(vm, ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NULL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;

    return upvalue;
}

ObjEnum *newEnum(VM *vm, ObjString *name) {
    ObjEnum *enumObj = ALLOCATE_OBJ(vm, ObjEnum, OBJ_ENUM);
    enumObj->name = name;
    initTable(&enumObj->values);

    return enumObj;
}

static void printFunction(ObjFunction *function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }

    printf("<fn %s>", function->name->str);
}

//TODO(Skyler): The next 4 functions currently cause memory leaks.
static char *functionToString(ObjFunction *function) {
    if (function->name == NULL) {
        return newCString("<script>");
    }
    
    char *ret = (char*)malloc(sizeof(char) * function->name->len + 6);
    snprintf(ret, function->name->len + 6, "<fn %s>", function->name->str);
    
    return ret;
}

static char *instanceToString(ObjInstance *instance) {
    char *ret = (char*)malloc(sizeof(char) * instance->objClass->name->len + 10);
    snprintf(ret, instance->objClass->name->len + 10, "%s instance", instance->objClass->name->str);
    
    return ret;
}

static char *scriptToString(ObjScript *script) {
    if (script->name == NULL) {
        return newCString("<library>");
    }
    
    char *ret = (char*)malloc(sizeof(char) * script->name->len + 11);
    snprintf(ret, script->name->len + 11, "<library %s>", script->name->str);
    
    return ret;
}

static char *enumToString(ObjEnum *objEnum) {
    char *enumString = malloc(sizeof(char) * (objEnum->name->len + 8));
    memcpy(enumString, "<enum ", 6);
    memcpy(enumString + 6, objEnum->name->str, objEnum->name->len);
    memcpy(enumString + 6 + objEnum->name->len, ">", 1);
    enumString[7 + objEnum->name->len] = '\0';

    return enumString;
}

char *objectType(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD:
        case OBJ_FUNCTION:
        case OBJ_CLOSURE: return newCString("function");
        case OBJ_CLASS: return newCString("class");
        case OBJ_INSTANCE: {
            ObjInstance *instance = AS_INSTANCE(value);
            return newCString(instance->objClass->name->str);
        }
        case OBJ_SCRIPT: return newCString("script");
        case OBJ_NATIVE: return newCString("cFunction");
        case OBJ_STRING: return newCString("string");
        case OBJ_UPVALUE: return newCString("upvalue");
        case OBJ_ENUM: return newCString("enum");
    }

    return newCString("unknown");
}

char *objectToString(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD: return functionToString(AS_BOUND_METHOD(value)->method->function);
        case OBJ_CLASS: return newCString(AS_CLASS(value)->name->str);
        case OBJ_CLOSURE: return functionToString(AS_CLOSURE(value)->function);
        case OBJ_FUNCTION: return functionToString(AS_FUNCTION(value));
        case OBJ_INSTANCE: return instanceToString(AS_INSTANCE(value));
        case OBJ_SCRIPT: return scriptToString(AS_SCRIPT(value));
        case OBJ_NATIVE: return newCString("<native fn>");
        case OBJ_STRING: return newCString(AS_STRING(value)->str);
        case OBJ_UPVALUE: return newCString("Should never happen.");
        case OBJ_ENUM: return enumToString(AS_ENUM(value));
    }
    
    return newCString("unknown object");
}
