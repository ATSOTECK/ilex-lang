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
        case OBJ_NATIVE: return newCString("nativeFunction");
        case OBJ_STRING: return newCString("string");
        case OBJ_UPVALUE: return newCString("upvalue");
    }

    return newCString("unknown");
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD: printFunction(AS_BOUND_METHOD(value)->method->function); break;
        case OBJ_CLASS: printf("%s", AS_CLASS(value)->name->str); break;
        case OBJ_CLOSURE: printFunction(AS_CLOSURE(value)->function); break;
        case OBJ_FUNCTION: printFunction(AS_FUNCTION(value)); break;
        case OBJ_INSTANCE: printf("%s instance", AS_INSTANCE(value)->objClass->name->str); break; //TODO: toString()
        case OBJ_LIBRARY: {
            ObjLibrary *library = AS_LIBRARY(value);
            if (library->name == NULL) {
                printf("<library>");
                break;
            }

            printf("<library %s>", library->name->str);
        } break;
        case OBJ_NATIVE: printf("<native fn>"); break;
        case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
        case OBJ_UPVALUE: printf("This should never be seen."); break;
    }
}
