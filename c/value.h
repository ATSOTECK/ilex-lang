//
// Created by Skyler on 11/12/21.
//

#ifndef C_VALUE_H
#define C_VALUE_H

#include "ilex.h"

#include <string.h>

typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray *array);
void writeValueArray(VM *vm, ValueArray *array, Value value);
void fillValueArray(VM *vm, int count, ValueArray *array, Value value);
void freeValueArray(VM *vm, ValueArray *array);

uint32_t hashValue(Value value);

char *valueType(Value value);

char *valueToString(Value value);
void printValue(Value value);

bool isFalsy(Value value);

#endif //C_VALUE_H
