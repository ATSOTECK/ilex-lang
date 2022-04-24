//
// Created by Skyler on 11/12/21.
//

#ifndef C_MEMORY_H
#define C_MEMORY_H

#include "common.h"
#include "object.h"

#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) (type*)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define SHRINK_ARRAY(previous, type, oldCount, count) (type*)reallocate(previous, sizeof(type) * (oldCount), sizeof(type) * count)

#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

void *reallocate(void *pointer, size_t oldSize, size_t newSize);
void markObject(Obj *obj);
void markValue(Value value);
void collectGarbage();
void freeObjects();

#endif //C_MEMORY_H
