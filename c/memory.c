//
// Created by Skyler on 11/12/21.
//

#include <stdlib.h>

#include "memory.h"
#include "vm.h"

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
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

static void freeObject(Obj *object) {
    switch (object->type) {
        case OBJ_FUNCTION: {
            ObjFunction *function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);
        } break;
        case OBJ_NATIVE: {
            FREE(ObjNative, object);
        } break;
        case OBJ_STRING: {
            ObjString *string = (ObjString*)object;
            FREE_ARRAY(char, string->str, string->len + 1);
            FREE(ObjString, object);
        } break;
    }
}

void freeObjects() {
    Obj *object = vm.objects;
    while (object != NULL) {
        Obj *next = object->next;
        freeObject(object);
        object = next;
    }
}
