//
// Created by Skyler on 05/7/22.
//

#include "lib_array.h"

#include "../memory.h"

static Value arrayLen(VM *vm, int argc, Value *args) {
    ObjArray *array = AS_ARRAY(args[0]);
    return NUMBER_VAL(array->data.count);
}

static Value arrayToStringLib(VM *vm, int argc, Value *args) {
    char *str = arrayToString(AS_ARRAY(args[0]));
    ObjString *ret = copyString(vm, str, (int)strlen(str));
    free(str);
    
    return OBJ_VAL(ret);
}

static Value arrayPush(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function push() expected 1 argument but got '%d'.", argc);
        return NULL_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    writeValueArray(vm, &array->data, args[1]);
    
    return ZERO_VAL;
}

static Value arrayPop(VM *vm, int argc, Value *args) {
    ObjArray *array = AS_ARRAY(args[0]);
    
    if (array->data.count == 0) {
        return NULL_VAL;
    }
    
    Value element = array->data.values[array->data.count - 1];
    --array->data.count;
    
    return element;
}

static Value arrayInsert(VM *vm, int argc, Value *args) {
    if (argc != 2) {
        runtimeError(vm, "Function insert() expected 2 arguments but got '%d'.", argc);
        return NULL_VAL;
    }
    
    if (!IS_NUMBER(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function insert() expected type 'number' for first argument but got '%s'.", str);
        free(str);
        return NULL_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    int idx = AS_NUMBER(args[1]);
    Value value = args[2];
    
    if (idx < 0 || idx > array->data.count) {
        runtimeError(vm, "Array index '%d' passed to insert() is out of bounds, array length is '%d'.", idx, array->data.count);
        return NULL_VAL;
    }
    
    
    if (array->data.capacity < array->data.count + 1) {
        int oldCapacity = array->data.capacity;
        array->data.capacity = GROW_CAPACITY(oldCapacity);
        array->data.values = GROW_ARRAY(vm, Value, array->data.values, oldCapacity, array->data.capacity);
    }
    
    ++array->data.count;
    
    for (int i = array->data.count - 1; i > idx; --i) {
        array->data.values[i] = array->data.values[i - 1];
    }
    
    array->data.values[idx] = value;
    
    return ZERO_VAL;
}

static Value arrayErase(VM *vm, int argc, Value *args) {
    if (argc == 0 || argc > 2) {
        runtimeError(vm, "Function erase() expected 1 or 2 arguments but got '%d'.", argc);
        return NULL_VAL;
    }
    
    if (!IS_NUMBER(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function erase() expected type 'number' for first argument but got '%s'.", str);
        free(str);
        return NULL_VAL;
    }
    
    if (argc == 2 && !IS_NUMBER(args[2])) {
        char *str = valueType(args[2]);
        runtimeError(vm, "Function erase() expected type 'number' for second argument but got '%s'.", str);
        free(str);
        return NULL_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    int idx = AS_NUMBER(args[1]);
    int count = argc == 2 ? (int)AS_NUMBER(args[2]) : 1;
    
    if (array->data.count == 0) {
        return NULL_VAL;
    }
    
    if (idx < 0 || idx > array->data.count) {
        runtimeError(vm, "Array index '%d' passed to erase() is out of bounds, array length is '%d'.", idx, array->data.count);
        return NULL_VAL;
    }
    
    if (count < 0) {
        count = 1;
    } else if (count > array->data.count) {
        count = array->data.count - idx;
    }
    
    if (array->data.count > 1 && idx != array->data.count) {
        for (int i = idx; i < array->data.count - count; i += count) {
            array->data.values[i] = array->data.values[i + count];
        }
    }
    
    array->data.count -= count;
    return ZERO_VAL;
}

static Value arrayRemove(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function remove() expected 1 argument but got '%d'.", argc);
        return NULL_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    Value value = args[1];
    bool found = false;
    
    if (array->data.count == 0) {
        return FALSE_VAL; // Or should this be true?
    }
    
    if (array->data.count > 1) {
        for (int i = 0; i < array->data.count - 1; i++) {
            if (!found && valuesEqual(value, array->data.values[i])) {
                found = true;
            }
            
            if (found) {
                array->data.values[i] = array->data.values[i + 1];
            }
        }
        
        if (!found && valuesEqual(value, array->data.values[array->data.count - 1])) {
            found = true;
        }
    } else {
        if (valuesEqual(value, array->data.values[0])) {
            found = true;
        }
    }
    
    if (found) {
        --array->data.count;
        return TRUE_VAL;
    }
    
    return FALSE_VAL;
}

static Value arrayContains(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function contains() expected 1 argument but got '%d'.", argc);
        return NULL_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    Value value = args[1];
    
    for (int i = 0; i < array->data.count; ++i) {
        if (valuesEqual(value, array->data.values[i])) {
            return TRUE_VAL;
        }
    }
    
    return FALSE_VAL;
}

void defineArrayFunctions(VM *vm) {
    defineNative(vm, "len", arrayLen, &vm->arrayFunctions);
    defineNative(vm, "toString", arrayToStringLib, &vm->arrayFunctions);
    defineNative(vm, "push", arrayPush, &vm->arrayFunctions);
    defineNative(vm, "pop", arrayPop, &vm->arrayFunctions);
    defineNative(vm, "insert", arrayInsert, &vm->arrayFunctions);
    defineNative(vm, "erase", arrayErase, &vm->arrayFunctions);
    defineNative(vm, "remove", arrayRemove, &vm->arrayFunctions);
    defineNative(vm, "contains", arrayContains, &vm->arrayFunctions);
}