//
// Created by Skyler on 11/12/21.
//

#ifndef C_MEMORY_H
#define C_MEMORY_H

#include "common.h"
#include "object.h"

#define ALLOCATE(vm, type, count) (type*)reallocate(vm, NULL, 0, sizeof(type) * (count))

#define FREE(vm, type, pointer) reallocate(vm, pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define SHRINK_CAPACITY(capacity) ((capacity) < 16 ? 8 : (capacity) / 2)

#define GROW_ARRAY(vm, type, pointer, oldCount, newCount) (type*)reallocate(vm, pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define SHRINK_ARRAY(vm, previous, type, oldCount, count) (type*)reallocate(vm, previous, sizeof(type) * (oldCount), sizeof(type) * count)

#define FREE_ARRAY(vm, type, pointer, oldCount) reallocate(vm, pointer, sizeof(type) * (oldCount), 0)

void *reallocate(VM *vm, void *pointer, size_t oldSize, size_t newSize);
void markObject(VM *vm, Obj *obj);
void markValue(VM *vm, Value value);
void collectGarbage(VM *vm);
void freeObjects(VM *vm);

#endif //C_MEMORY_H
