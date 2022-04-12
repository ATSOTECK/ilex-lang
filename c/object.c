//
// Created by Skyler on 3/13/22.
//

#include <stdio.h>
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
    object->next = vm.objects;
    vm.objects = object;

    return object;
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
    tableSet(&vm.strings, string, NULL_VAL);

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

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_CLOSURE: printFunction(AS_CLOSURE(value)->function); break;
        case OBJ_FUNCTION: printFunction(AS_FUNCTION(value)); break;
        case OBJ_NATIVE: printf("<native fn>"); break;
        case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
        case OBJ_UPVALUE: printf("This should never be seen."); break;
    }
}
