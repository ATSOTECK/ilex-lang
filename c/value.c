//
// Created by Skyler on 11/12/21.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

bool valuesEqual(Value a, Value b) {
    // IEEE 754 compliance is for NERDS.
    /*
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        return AS_NUMBER(a) == AS_NUMBER(b);
    }
    */

    return a == b;
}

void initValueArray(ValueArray *array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(VM *vm, ValueArray *array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(vm, Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(VM *vm, ValueArray *array) {
    FREE_ARRAY(vm, Value, array->values, array->capacity);
    initValueArray(array);
}

char *valueType(VM *vm, Value value) {
    if (IS_BOOL(value)) {
        return newCString(vm, "bool");
    } else if (IS_NUMBER(value)) {
        return newCString(vm, "number");
    } else if (IS_NULL(value)) {
        return newCString(vm, "null");
    } else if (IS_OBJ(value)) {
        return objectType(vm, value);
    }

    return newCString(vm, "unknown");
}

char *valueToString(VM *vm, Value value) {
    if (IS_BOOL(value)) {
        return newCString(vm, AS_BOOL(value) ? "true" : "false");
    } else if (IS_NULL(value)) {
        return newCString(vm, "null");
    } else if (IS_NUMBER(value)) {
        double num = AS_NUMBER(value);
        int len = snprintf(NULL, 0, "%.15g", num) + 1;
        char *ret = (char*)malloc(sizeof(char) * len);
        snprintf(ret, len, "%.15g", num);
        
        return ret;
    } else if (IS_OBJ(value)) {
        return objectToString(vm, value);
    }
    
    // Should never be reached.
    return newCString(vm, "unknown value");
}

void printValue(VM *vm, Value value) {
    char *str = valueToString(vm, value);
    printf("%s", str);
    free(str);
}
