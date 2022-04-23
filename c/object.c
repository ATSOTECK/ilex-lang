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

#define ALLOCATE_OBJ(type, objectType) (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
    Obj *object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    object->next = vm.objects;
    vm.objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

    return object;
}

ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method) {
    ObjBoundMethod *bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClass *newClass(ObjString *name) {
    ObjClass* objClass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    objClass->name = name;
    initTable(&objClass->methods);

    return objClass;
}

ObjClosure *newClosure(ObjFunction *function) {
    ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; ++i) {
        upvalues[i] = NULL;
    }

    ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;

    return closure;
}

ObjFunction *newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);

    return function;
}

ObjInstance *newInstance(ObjClass *objClass) {
    ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->objClass = objClass;
    initTable(&instance->fields);

    return instance;
}

ObjNative *newNative(NativeFn function) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;

    return native;
}

static ObjString *allocateString(char *str, int len, uint32_t hash) {
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->len = len;
    string->str = str;
    string->hash = hash;

    push(OBJ_VAL(string));
    tableSet(&vm.strings, string, NULL_VAL);
    pop();

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

char *newCString(char *str) {
    size_t len = strlen(str);

    char* ret = ALLOCATE(char, len + 1);
    memcpy(ret, str, len);
    ret[len] = '\0';

    return ret;
}

ObjString *takeString(char *str, int len) {
    uint32_t hash = hashString(str, len);
    ObjString *interned = tableFindString(&vm.strings, str, len, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, str, len + 1);
        return interned;
    }
    return allocateString(str, len, hash);
}

ObjString *copyString(const char *str, int len) {
    uint32_t hash = hashString(str, len);
    ObjString *interned = tableFindString(&vm.strings, str, len, hash);
    if (interned != NULL) {
        return interned;
    }
    char *heapStr = ALLOCATE(char, len + 1);
    memcpy(heapStr, str, len);
    heapStr[len] = '\0';

    return allocateString(heapStr, len, hash);
}

ObjUpvalue *newUpvalue(Value *slot) {
    ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NULL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;

    return upvalue;
}

static void printFunction(ObjFunction *function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }

    printf("<fn %s>", function->name->str);
}

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

static char *libraryToString(ObjLibrary *library) {
    if (library->name == NULL) {
        return newCString("<library>");
    }
    
    char *ret = (char*)malloc(sizeof(char) * library->name->len + 11);
    snprintf(ret, library->name->len + 11, "<library %s>", library->name->str);
    
    return ret;
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
        case OBJ_LIBRARY: return newCString("library");
        case OBJ_NATIVE: return newCString("cFunction");
        case OBJ_STRING: return newCString("string");
        case OBJ_UPVALUE: return newCString("upvalue");
    }

    return newCString("unknown");
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD: printFunction(AS_BOUND_METHOD(value)->method->function); return;
        case OBJ_CLASS: printf("%s", AS_CLASS(value)->name->str); return;
        case OBJ_CLOSURE: printFunction(AS_CLOSURE(value)->function); return;
        case OBJ_FUNCTION: printFunction(AS_FUNCTION(value)); return;
        case OBJ_INSTANCE: printf("%s instance", AS_INSTANCE(value)->objClass->name->str); return; //TODO: toString()
        case OBJ_LIBRARY: printf("%s", libraryToString(AS_LIBRARY(value))); return;
        case OBJ_NATIVE: printf("<native fn>"); return;
        case OBJ_STRING: printf("%s", AS_CSTRING(value)); return;
        case OBJ_UPVALUE: printf("Should never happen."); return;
    }
    
    printf("unknown object");
}

char *objectToString(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD: return functionToString(AS_BOUND_METHOD(value)->method->function);
        case OBJ_CLASS: return newCString(AS_CLASS(value)->name->str);
        case OBJ_CLOSURE: return functionToString(AS_CLOSURE(value)->function);
        case OBJ_FUNCTION: return functionToString(AS_FUNCTION(value));
        case OBJ_INSTANCE: return instanceToString(AS_INSTANCE(value));
        case OBJ_LIBRARY: return libraryToString(AS_LIBRARY(value));
        case OBJ_NATIVE: return newCString("<native fn>");
        case OBJ_STRING: return newCString(AS_STRING(value)->str);
        case OBJ_UPVALUE: return newCString("Should never happen.");
    }
    
    return newCString("unknown object");
}
