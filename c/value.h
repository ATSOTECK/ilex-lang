//
// Created by Skyler on 11/12/21.
//

#ifndef C_VALUE_H
#define C_VALUE_H

#include "common.h"

typedef double Value;

typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);

void printValue(Value value);

#endif //C_VALUE_H
