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

char *newCString(VM *vm, char *str) {
    size_t len = strlen(str);

    char* ret = ALLOCATE(vm, char, len + 1);
    memcpy(ret, str, len);
    ret[len] = '\0';

    return ret;
}

char *newCStringLen(VM *vm, const char *str, int len) {
    char* ret = ALLOCATE(vm, char, len + 1);
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
static char *functionToString(VM *vm, ObjFunction *function) {
    if (function->name == NULL) {
        return newCString(vm, "<script>");
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

static char *libraryToString(VM *vm, ObjLibrary *library) {
    if (library->name == NULL) {
        return newCString(vm, "<library>");
    }
    
    char *ret = (char*)malloc(sizeof(char) * library->name->len + 11);
    snprintf(ret, library->name->len + 11, "<library %s>", library->name->str);
    
    return ret;
}

static char *enumToString(VM *vm, ObjEnum *objEnum) {
    char *enumString = malloc(sizeof(char) * (objEnum->name->len + 8));
    memcpy(enumString, "<enum ", 6);
    memcpy(enumString + 6, objEnum->name->str, objEnum->name->len);
    memcpy(enumString + 6 + objEnum->name->len, ">", 1);
    enumString[7 + objEnum->name->len] = '\0';

    return enumString;
}

char *objectType(VM *vm, Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD:
        case OBJ_FUNCTION:
        case OBJ_CLOSURE: return newCString(vm, "function");
        case OBJ_CLASS: return newCString(vm, "class");
        case OBJ_INSTANCE: {
            ObjInstance *instance = AS_INSTANCE(value);
            return newCString(vm, instance->objClass->name->str);
        }
        case OBJ_LIBRARY: return newCString(vm, "library");
        case OBJ_NATIVE: return newCString(vm, "cFunction");
        case OBJ_STRING: return newCString(vm, "string");
        case OBJ_UPVALUE: return newCString(vm, "upvalue");
        case OBJ_ENUM: return newCString(vm, "enum");
    }

    return newCString(vm, "unknown");
}

char *objectToString(VM *vm, Value value) {
    //TODO(Skyler): Don't use GC.
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD: return functionToString(vm, AS_BOUND_METHOD(value)->method->function);
        case OBJ_CLASS: return newCString(vm, AS_CLASS(value)->name->str);
        case OBJ_CLOSURE: return functionToString(vm, AS_CLOSURE(value)->function);
        case OBJ_FUNCTION: return functionToString(vm, AS_FUNCTION(value));
        case OBJ_INSTANCE: return instanceToString(AS_INSTANCE(value));
        case OBJ_LIBRARY: return libraryToString(vm, AS_LIBRARY(value));
        case OBJ_NATIVE: return newCString(vm, "<native fn>");
        case OBJ_STRING: return newCString(vm, AS_STRING(value)->str);
        case OBJ_UPVALUE: return newCString(vm, "Should never happen.");
        case OBJ_ENUM: return enumToString(vm, AS_ENUM(value));
    }
    
    return newCString(vm, "unknown object");
}
