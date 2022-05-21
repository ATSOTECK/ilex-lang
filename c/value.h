//
// Created by Skyler on 11/12/21.
//

#ifndef C_VALUE_H
#define C_VALUE_H

#include "common.h"

#include "ilex_include.h"

#include <string.h>

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)

#define TAG_NULL  1 // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE  3 // 11.
#define TAG_ERR   4 // 10.

typedef uint64_t Value;

#define IS_BOOL(value)      (((value) | 1u) == TRUE_VAL)
#define IS_NULL(value)      ((value) == NULL_VAL)
#define IS_NUMBER(value)    (((value) & QNAN) != QNAN)
#define IS_ERR(value)       ((value) == ERROR_VAL)
#define IS_OBJ(value)       (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)      ((value) == TRUE_VAL)
#define AS_NUMBER(value)    valueToNum(value)
#define AS_OBJ(value)       ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))


#define BOOL_VAL(b)     ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL       ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL        ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NULL_VAL        ((Value)(uint64_t)(QNAN | TAG_NULL))
#define ERROR_VAL       ((Value)(uint64_t)(QNAN | TAG_ERR))
#define NUMBER_VAL(num) numToValue(num)
#define ZERO_VAL        numToValue(0)
#define OBJ_VAL(obj)    (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

typedef union {
    uint64_t bits64;
    uint32_t bits32[2];
    double num;
} Double;

static inline double valueToNum(Value value) {
    Double data;
    data.bits64 = value;
    return data.num;
}

static inline Value numToValue(double num) {
    Double data;
    data.num = num;
    return data.bits64;
}

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

char *valueType(Value value);

char *valueToString(Value value);
void printValue(Value value);

#endif //C_VALUE_H
