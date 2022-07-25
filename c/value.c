//
// Created by Skyler on 11/12/21.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

static bool arraysEqual(ObjArray *a, ObjArray *b) {
    if (a->data.count != b->data.count) {
        return false;
    }
    
    for (int i = 0; i < a->data.count; ++i) {
        if (!valuesEqual(a->data.values[i], b->data.values[i])) {
            return false;
        }
    }
    
    return true;
}

bool valuesEqual(Value a, Value b) {
    // IEEE 754 compliance is for NERDS.
    /*
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        return AS_NUMBER(a) == AS_NUMBER(b);
    }
    */
    
    if (IS_OBJ(a) && IS_OBJ(b)) {
        Obj *aObj = AS_OBJ(a);
        if (aObj->type != AS_OBJ(b)->type) {
            return false;
        }
        
        switch (aObj->type) {
            case OBJ_ARRAY: return arraysEqual(AS_ARRAY(a), AS_ARRAY(b));
            default: break;
        }
    }

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

void fillValueArray(VM *vm, int count, ValueArray *array, Value value) {
    if (array->capacity < count) {
        int oldCapacity = array->capacity;
        array->capacity = count;
        array->values = GROW_ARRAY(vm, Value, array->values, oldCapacity, array->capacity);
    }

    for (int i = 0; i < count; ++i) {
        array->values[i] = value;
    }

    array->count = count;
}

void freeValueArray(VM *vm, ValueArray *array) {
    FREE_ARRAY(vm, Value, array->values, array->capacity);
    initValueArray(array);
}

static inline uint32_t hashBits(uint64_t hash) {
    // From v8's ComputeLongHash() which in turn cites:
    // Thomas Wang, Integer Hash Functions.
    hash = ~hash + (hash << 18);  // hash = (hash << 18) - hash - 1;
    hash = hash ^ (hash >> 31);
    hash = hash * 21;  // hash = (hash + (hash << 2)) + (hash << 4);
    hash = hash ^ (hash >> 11);
    hash = hash + (hash << 6);
    hash = hash ^ (hash >> 22);
    return (uint32_t) (hash & 0x3fffffff);
}

static uint32_t hashObject(Obj *obj) {
    switch (obj->type) {
        case OBJ_STRING: {
            return ((ObjString *)obj)->hash;
        }
        default: {
#ifdef DEBUG_PRINT_CODE
            printf("Object: ");
            printValue(OBJ_VAL(obj));
            printf(" is not hashable!\n");
            exit(1);
#endif
            return -1;
        }
    }
}

uint32_t hashValue(Value value) {
    if (IS_OBJ(value)) {
        return hashObject(AS_OBJ(value));
    }
    
    return hashBits(value);
}

char *valueType(Value value) {
    if (IS_BOOL(value)) {
        return newCString("bool");
    } else if (IS_NUMBER(value)) {
        return newCString("number");
    } else if (IS_NULL(value)) {
        return newCString("null");
    } else if (IS_OBJ(value)) {
        return objectType(value);
    }

    return newCString("unknown");
}

char *valueToString(Value value) {
    //TODO(Skyler): Don't use GC.
    if (IS_BOOL(value)) {
        return newCString(AS_BOOL(value) ? "true" : "false");
    } else if (IS_NULL(value)) {
        return newCString("null");
    } else if (IS_NUMBER(value)) {
        double num = AS_NUMBER(value);
        int len = snprintf(NULL, 0, "%.15g", num) + 1;
        char *ret = (char*)malloc(sizeof(char) * len);
        snprintf(ret, len, "%.15g", num);
        
        return ret;
    } else if (IS_OBJ(value)) {
        return objectToString(value);
    }
    
    // Should never be reached.
    return newCString("unknown value");
}

void printValue(Value value) {
    char *str = valueToString(value);
    printf("%s", str);
    free(str);
}
