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

void *reallocate(VM *vm, void *pointer, size_t oldSize, size_t newSize) {
    vm->bytesAllocated += newSize - oldSize;
    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage(vm);
#endif
        if (vm->bytesAllocated > vm->nextGC) {
            collectGarbage(vm);
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

void markObject(VM *vm, Obj *obj) {
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

    if (vm->grayCapacity < vm->grayCount + 1) {
        vm->grayCapacity = GROW_CAPACITY(vm->grayCapacity);
        vm->grayStack = (Obj**)realloc(vm->grayStack, sizeof(Obj*) * vm->grayCapacity);

        if (vm->grayStack == NULL) {
            exit(69);
        }
    }

    vm->grayStack[vm->grayCount++] = obj;
}

void markValue(VM *vm, Value value) {
    if (IS_OBJ(value)) {
        markObject(vm, AS_OBJ(value));
    }
}

static void markArray(VM *vm, const ValueArray *array) {
    for (int i = 0; i < array->count; ++i) {
        markValue(vm, array->values[i]);
    }
}

static void blackenObject(VM *vm, Obj *obj) {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)obj);
    printValue(OBJ_VAL(obj));
    printf("\n");
#endif

    switch (obj->type) {
        case OBJ_BOUND_METHOD: {
            const ObjBoundMethod *bound = (ObjBoundMethod*)obj;
            markValue(vm, bound->receiver);
            markObject(vm, (Obj*)bound->method);
        } break;
        case OBJ_CLASS: {
            ObjClass *objClass = (ObjClass*)obj;
            markTable(vm, &objClass->methods);
            markTable(vm, &objClass->privateMethods);
            markTable(vm, &objClass->abstractMethods);
            markTable(vm, &objClass->staticVars);
            markTable(vm, &objClass->staticConsts);
            markTable(vm, &objClass->fields);
            markTable(vm, &objClass->privateFields);
            markObject(vm, (Obj*)objClass->name);
            markObject(vm, (Obj*)objClass->superClass);
        } break;
        case OBJ_CLOSURE: {
            const ObjClosure *closure = (ObjClosure*)obj;
            markObject(vm, (Obj*)closure->function);
            for (int i = 0; i < closure->upvalueCount; ++i) {
                markObject(vm, (Obj*)closure->upvalues[i]);
            }
        } break;
        case OBJ_FUNCTION: {
            ObjFunction *function = (ObjFunction*)obj;
            markObject(vm, (Obj*)function->name);
            markArray(vm, &function->chunk.constants);
        } break;
        case OBJ_INSTANCE: {
            ObjInstance *instance = (ObjInstance*)obj;
            markObject(vm, (Obj*)instance->objClass);
            markTable(vm, &instance->fields);
            markTable(vm, &instance->privateFields);
        } break;
        case OBJ_UPVALUE: {
            markValue(vm, ((ObjUpvalue*)obj)->closed);
        } break;
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
        case OBJ_ENUM: {
            ObjEnum *objEnum = (ObjEnum*)obj;
            markObject(vm, (Obj*)objEnum->name);
            markTable(vm, &objEnum->values);
        } break;
        case OBJ_ARRAY: {
            ObjArray *array = (ObjArray*)obj;
            markArray(vm, &array->data);
        } break;
        case OBJ_MAP: {
            ObjMap *map = (ObjMap*)obj;
            markMap(vm, map);
        } break;
        case OBJ_SET: {
            ObjSet *set = (ObjSet*)obj;
            markSet(vm, set);
        } break;
        case OBJ_SCRIPT: {
            ObjScript *script = (ObjScript*)obj;
            markObject(vm, (Obj*)script->name);
            markObject(vm, (Obj*)script->path);
            markTable(vm, &script->values);
        } break;
        case OBJ_FILE:
            break;
        case OBJ_ABSTRACT: {
            ObjAbstract *abstract = (ObjAbstract*)obj;
            markTable(vm, &abstract->values);
        } break;
        default: break;
    }
}

static void freeObject(VM *vm, Obj *obj) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)obj, obj->type);
#endif

    switch (obj->type) {
        case OBJ_BOUND_METHOD: {
            FREE(vm, ObjBoundMethod, obj);
        } break;
        case OBJ_CLASS: {
            ObjClass *objClass = (ObjClass*)obj;
            freeTable(vm, &objClass->methods);
            freeTable(vm, &objClass->privateMethods);
            freeTable(vm, &objClass->abstractMethods);
            freeTable(vm, &objClass->staticVars);
            freeTable(vm, &objClass->staticConsts);
            freeTable(vm, &objClass->fields);
            freeTable(vm, &objClass->privateFields);
            FREE(vm, ObjClass, obj);
        } break;
        case OBJ_CLOSURE: {
            ObjClosure *closure = (ObjClosure*)obj;
            FREE_ARRAY(vm, ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(vm, ObjClosure, obj);
        } break;
        case OBJ_FUNCTION: {
            ObjFunction *function = (ObjFunction*)obj;
            freeChunk(vm, &function->chunk);
            FREE(vm, ObjFunction, obj);
        } break;
        case OBJ_INSTANCE: {
            ObjInstance *instance = (ObjInstance*)obj;
            freeTable(vm, &instance->fields);
            FREE(vm, ObjInstance, obj);
        } break;
        case OBJ_NATIVE: {
            FREE(vm, ObjNative, obj);
        } break;
        case OBJ_STRING: {
            ObjString *string = (ObjString*)obj;
            FREE_ARRAY(vm, char, string->str, string->len + 1);
            FREE(vm, ObjString, obj);
        } break;
        case OBJ_UPVALUE: {
            FREE(vm, ObjUpvalue, obj);
        } break;
        case OBJ_ENUM: {
            ObjEnum *objEnum = (ObjEnum*)obj;
            freeTable(vm, &objEnum->values);
            FREE(vm, ObjEnum, obj);
        } break;
        case OBJ_ARRAY: {
            ObjArray *array = (ObjArray*)obj;
            freeValueArray(vm, &array->data);
            FREE(vm, ObjArray, obj);
        } break;
        case OBJ_MAP: {
            ObjMap *map = (ObjMap*)obj;
            FREE_ARRAY(vm, MapItem, map->items, map->capacity + 1);
            FREE(vm, ObjMap, obj);
        } break;
        case OBJ_SET: {
            ObjSet *set = (ObjSet*)obj;
            FREE_ARRAY(vm, SetItem, set->items, set->capacity + 1);
            FREE(vm, ObjSet, obj);
        } break;
        case OBJ_SCRIPT: {
            ObjScript *script = (ObjScript*)obj;
            freeTable(vm, &script->values);
            FREE(vm, ObjScript, obj);
        } break;
        case OBJ_FILE: {
            FREE(vm, ObjFile, obj);
        } break;
        case OBJ_ABSTRACT: {
            ObjAbstract *abstract = (ObjAbstract*)obj;
            abstract->feeFn(vm, abstract);
            freeTable(vm, &abstract->values);
            FREE(vm, ObjAbstract, obj);
        } break;
    }
}

static void markRoots(VM *vm) {
    for (const Value *slot = vm->stack; slot < vm->stackTop; slot++) {
        markValue(vm, *slot);
    }

    for (int i = 0; i < vm->frameCount; ++i) {
        markObject(vm, (Obj*)vm->frames[i].closure);
    }

    for (ObjUpvalue *upvalue = vm->openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        markObject(vm, (Obj*)upvalue);
    }

    markTable(vm, &vm->scripts);
    markTable(vm, &vm->globals);
    markTable(vm, &vm->consts);
    markTable(vm, &vm->numberFunctions);
    markTable(vm, &vm->stringFunctions);
    markTable(vm, &vm->arrayFunctions);
    markTable(vm, &vm->mapFunctions);
    markTable(vm, &vm->setFunctions);
    markTable(vm, &vm->enumFunctions);
    markCompilerRoots(vm);
    markObject(vm, (Obj*)vm->initString);
    markObject(vm, (Obj*)vm->scriptName);
}

static void traceRefs(VM *vm) {
    while (vm->grayCount > 0) {
        Obj *obj = vm->grayStack[--vm->grayCount];
        blackenObject(vm, obj);
    }
}

static void sweep(VM *vm) {
    Obj *prev = NULL;
    Obj *obj = vm->objects;
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
                vm->objects = obj;
            }

            freeObject(vm, unreached);
        }
    }
}

void collectGarbage(VM *vm) {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm->bytesAllocated;
#endif

    ++vm->gcRuns;
    markRoots(vm);
    traceRefs(vm);
    tableRemoveWhite(&vm->strings);
    sweep(vm);

    vm->nextGC = vm->bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
           before - vm->bytesAllocated, before, vm->bytesAllocated,
           vm->nextGC);
#endif
}

void freeObjects(VM *vm) {
    Obj *object = vm->objects;
    while (object != NULL) {
        Obj *next = object->next;
        freeObject(vm, object);
        object = next;
    }

    free(vm->grayStack);
}
