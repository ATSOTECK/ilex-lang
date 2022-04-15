//
// Created by Skyler on 11/12/21.
//

#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
    vm.bytesAllocated += newSize - oldSize;
    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif
        if (vm.bytesAllocated > vm.nextGC) {
            collectGarbage();
        }
    }

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void *mem = realloc(pointer, newSize);
    if (mem == NULL) {
        exit(69);
    }

    return mem;
}

void markObject(Obj *obj) {
    if (obj == NULL) {
        return;
    }

    if (obj->isMarked) {
        return;
    }

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)obj);
    printValue(OBJ_VAL(obj));
    printf("\n");
#endif

    obj->isMarked = true;

    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj**)realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);

        if (vm.grayStack == NULL) {
            exit(69);
        }
    }

    vm.grayStack[vm.grayCount++] = obj;
}

void markValue(Value value) {
    if (IS_OBJ(value)) {
        markObject(AS_OBJ(value));
    }
}

static void markArray(ValueArray *array) {
    for (int i = 0; i < array->count; ++i) {
        markValue(array->values[i]);
    }
}

static void blackenObject(Obj *obj) {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)obj);
    printValue(OBJ_VAL(obj));
    printf("\n");
#endif

    switch (obj->type) {
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod *bound = (ObjBoundMethod*)obj;
            markValue(bound->receiver);
            markObject((Obj*)bound->method);
        } break;
        case OBJ_CLASS: {
            ObjClass *objClass = (ObjClass*)obj;
            markTable(&objClass->methods);
            markObject((Obj*)objClass->name);
        } break;
        case OBJ_CLOSURE: {
            ObjClosure *closure = (ObjClosure*)obj;
            markObject((Obj*)closure->function);
            for (int i = 0; i < closure->upvalueCount; ++i) {
                markObject((Obj*)closure->upvalues[i]);
            }
        } break;
        case OBJ_FUNCTION: {
            ObjFunction *function = (ObjFunction*)obj;
            markObject((Obj*)function->name);
            markArray(&function->chunk.constants);
        } break;
        case OBJ_INSTANCE: {
            ObjInstance *instance = (ObjInstance*)obj;
            markObject((Obj*)instance->objClass);
            markTable(&instance->fields);
        } break;
        case OBJ_UPVALUE: {
            markValue(((ObjUpvalue*)obj)->closed);
        } break;
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    }
}

static void freeObject(Obj *object) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif

    switch (object->type) {
        case OBJ_BOUND_METHOD: {
            FREE(ObjBoundMethod, object);
        } break;
        case OBJ_CLASS: {
            ObjClass *objClass = (ObjClass*)object;
            freeTable(&objClass->methods);
            FREE(ObjClass, object);
        } break;
        case OBJ_CLOSURE: {
            ObjClosure *closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(ObjClosure, object);
        } break;
        case OBJ_FUNCTION: {
            ObjFunction *function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);
        } break;
        case OBJ_INSTANCE: {
            ObjInstance *instance = (ObjInstance*)object;
            freeTable(&instance->fields);
            FREE(ObjInstance, object);
        } break;
        case OBJ_NATIVE: {
            FREE(ObjNative, object);
        } break;
        case OBJ_STRING: {
            ObjString *string = (ObjString*)object;
            FREE_ARRAY(char, string->str, string->len + 1);
            FREE(ObjString, object);
        } break;
        case OBJ_UPVALUE: {
            FREE(ObjUpvalue, object);
        } break;
    }
}

static void markRoots() {
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }

    for (int i = 0; i < vm.frameCount; ++i) {
        markObject((Obj*)vm.frames[i].closure);
    }

    for (ObjUpvalue *upvalue = vm.openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        markObject((Obj*)upvalue);
    }

    markTable(&vm.globals);
    markCompilerRoots();
    markObject((Obj*)vm.initString);
}

static void traceRefs() {
    while (vm.grayCount > 0) {
        Obj *obj = vm.grayStack[--vm.grayCount];
        blackenObject(obj);
    }
}

static void sweep() {
    Obj *prev = NULL;
    Obj *obj = vm.objects;
    while (obj != NULL) {
        if (obj->isMarked) {
            obj->isMarked = false;
            prev = obj;
            obj = obj->next;
        } else {
            Obj *unreached = obj;
            obj = obj->next;
            if (prev != NULL) {
                prev->next = obj;
            } else {
                vm.objects = obj;
            }

            freeObject(unreached);
        }
    }
}

void collectGarbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.bytesAllocated;
#endif

    markRoots();
    traceRefs();
    tableRemoveWhite(&vm.strings);
    sweep();

    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
           before - vm.bytesAllocated, before, vm.bytesAllocated,
           vm.nextGC);
#endif
}

void freeObjects() {
    Obj *object = vm.objects;
    while (object != NULL) {
        Obj *next = object->next;
        freeObject(object);
        object = next;
    }

    free(vm.grayStack);
}
