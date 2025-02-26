//
// Created by Skyler on 05/7/22.
//

#include "type_array.h"

#include "../memory.h"

#include <math.h>
#include <stdlib.h>

static Value arrayLen(VM *vm, int argc, const Value *args) {
    ObjArray *array = AS_ARRAY(args[0]);
    return NUMBER_VAL(array->data.count);
}

static Value arrayToStringLib(VM *vm, int argc, const Value *args) {
    char *str = arrayToString(AS_ARRAY(args[0]));
    ObjString *ret = copyString(vm, str, (int)strlen(str));
    free(str);
    
    return OBJ_VAL(ret);
}

static Value arrayMake(VM *vm, int argc, const Value *args) {
    if (argc == 0 || argc > 2) {
        runtimeError(vm, "Function make() expected 1 or 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    ObjArray *array = AS_ARRAY(args[0]);
    freeValueArray(vm, &array->data); // Should the array be freed or should the count be set to 0?
    initValueArray(&array->data);

    if (!IS_NUMBER(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function make() expected type 'number' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    int count = (int)AS_NUMBER(args[1]);
    Value value = argc == 2 ? args[2] : ZERO_VAL;
    fillValueArray(vm, count, &array->data, value);

    return ZERO_VAL;
}

static Value arrayFill(VM *vm, int argc, const Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function fill() expected 0 or 1 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    ObjArray *array = AS_ARRAY(args[0]);
    Value value = argc == 1 ? args[1] : ZERO_VAL;
    fillValueArray(vm, array->data.count, &array->data, value);

    return ZERO_VAL;
}

static Value arrayPush(VM *vm, int argc, const Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function push() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    writeValueArray(vm, &array->data, args[1]);
    
    return ZERO_VAL;
}

static Value arrayPop(VM *vm, int argc, const Value *args) {
    ObjArray *array = AS_ARRAY(args[0]);
    
    if (array->data.count == 0) {
        return NULL_VAL;
    }
    
    Value element = array->data.values[array->data.count - 1];
    --array->data.count;
    
    return element;
}

static Value arrayInsert(VM *vm, int argc, const Value *args) {
    if (argc != 2) {
        runtimeError(vm, "Function insert() expected 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!IS_NUMBER(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function insert() expected type 'number' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    int idx = AS_NUMBER(args[1]);
    Value value = args[2];
    
    if (idx < 0 || idx > array->data.count) {
        runtimeError(vm, "Array index '%d' passed to insert() is out of bounds, array length is '%d'.", idx, array->data.count);
        return ERROR_VAL;
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

static Value arrayErase(VM *vm, int argc, const Value *args) {
    if (argc == 0 || argc > 2) {
        runtimeError(vm, "Function erase() expected 1 or 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!IS_NUMBER(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function erase() expected type 'number' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }
    
    if (argc == 2 && !IS_NUMBER(args[2])) {
        char *str = valueType(args[2]);
        runtimeError(vm, "Function erase() expected type 'number' for second argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    int idx = AS_NUMBER(args[1]);
    int count = argc == 2 ? (int)AS_NUMBER(args[2]) : 1;
    
    if (array->data.count == 0) {
        return NULL_VAL;
    }
    
    if (idx < 0 || idx > array->data.count) {
        runtimeError(vm, "Array index '%d' passed to erase() is out of bounds, array length is '%d'.", idx, array->data.count);
        return ERROR_VAL;
    }
    
    if (count < 0) {
        count = 1;
    } else if (idx + count > array->data.count) {
        count = array->data.count - idx;
    }
    
    if (array->data.count > 1 && idx != array->data.count) {
        for (int i = idx; i < array->data.count - count; ++i) {
            array->data.values[i] = array->data.values[i + count];
        }
    }
    
    array->data.count -= count;
    return ZERO_VAL;
}

static Value arrayRemove(VM *vm, int argc, const Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function remove() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
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

static Value arrayContains(VM *vm, int argc, const Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function contains() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
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

static Value arrayIndexOf(VM *vm, int argc, const Value *args) {
    if (argc == 0 || argc > 2) {
        runtimeError(vm, "Function indexOf() expected 1 or 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    Value value = args[1];
    int startIdx = 0;
    
    if (argc == 2) {
        if (!IS_NUMBER(args[2])) {
            char *str = valueType(args[2]);
            runtimeError(vm, "Function indexOf() expected type 'number' for second argument but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }
        
        startIdx = AS_NUMBER(args[2]);
        
        if (startIdx < 0) {
            startIdx = 0;
        } else if (startIdx > array->data.count) {
            return NUMBER_VAL(-1);
        }
    }
    
    for (int i = startIdx; i < array->data.count; ++i) {
        if (valuesEqual(value, array->data.values[i])) {
            return NUMBER_VAL(i);
        }
    }
    
    return NUMBER_VAL(-1);
}

static Value arrayReverse(VM *vm, int argc, const Value *args) {
    ObjArray *array = AS_ARRAY(args[0]);
    int len = array->data.count;
    
    for (int i = 0; i < len / 2; ++i) {
        Value tmp = array->data.values[i];
        array->data.values[i] = array->data.values[len - i - 1];
        array->data.values[len - i - 1] = tmp;
    }
    
    return ZERO_VAL;
}

static int partition(const ObjArray *array, int start, int end, bool asc) {
    int pivot_index = (int)floor(start + end) / 2;
    
    double pivot =  AS_NUMBER(array->data.values[pivot_index]);
    
    int i = start - 1;
    int j = end + 1;
    
    for (;;) {
        if (asc) {
            do {
                i = i + 1;
            } while (AS_NUMBER(array->data.values[i]) < pivot);
    
            do {
                j = j - 1;
            } while (AS_NUMBER(array->data.values[j]) > pivot);
        } else {
            do {
                i = i + 1;
            } while (AS_NUMBER(array->data.values[i]) > pivot);
    
            do {
                j = j - 1;
            } while (AS_NUMBER(array->data.values[j]) < pivot);
        }
        
        if (i >= j) {
            return j;
        }
        
        Value tmp = array->data.values[i];
        
        array->data.values[i] = array->data.values[j];
        array->data.values[j] = tmp;
    }
}

static void quickSort(ObjArray *array, int start, int end, bool asc) {
    while (start < end) {
        int part = partition(array, start, end, asc);
        
        if (part - start < end - part) {
            quickSort(array, start, part, asc);
            
            start = start + 1;
        } else {
            quickSort(array, part + 1, end, asc);
            
            end = end - 1;
        }
    }
}

static Value arraySort(VM *vm, int argc, const Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function sort() expected 0 or 1 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    int len = array->data.count;
    bool asc = true;
    
    if (argc == 1) {
        if (!IS_BOOL(args[1])) {
            char *str = valueType(args[1]);
            runtimeError(vm, "Function sort() expected type 'bool' for first argument but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }
        
        asc = AS_BOOL(args[1]);
    }
    
    if (len == 0) {
        return ZERO_VAL;
    }
    
    for (int i = 0; i < len; ++i) {
        if (!IS_NUMBER(array->data.values[i])) {
            char *str = valueType(array->data.values[i]);
            runtimeError(vm, "Function sort() expected array of numbers but found type '%s' at index '%d'.", str, i);
            free(str);
            
            return ERROR_VAL;
        }
    }
    
    quickSort(array, 0, len - 1, asc);
    
    return ZERO_VAL;
}

static Value arrayJoin(VM *vm, int argc, const Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function join() expected 0 or 1 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    int len = array->data.count;
    
    if (len == 0) {
        return OBJ_VAL(copyString(vm, "", 0));
    }
    
    char *delim = ", ";
    
    if (argc == 1) {
        if (!IS_STRING(args[1])) {
            char *str = valueType(args[1]);
            runtimeError(vm, "Function join() expected type 'string' for first argument but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }
        
        delim = AS_CSTRING(args[1]);
    }
    
    char *output;
    char *fullString = NULL;
    int strLen = 0;
    int delimLen = (int)strlen(delim);
    
    for (int i = 0; i < len; ++i) {
        if (IS_STRING(array->data.values[i])) {
            output = AS_CSTRING(array->data.values[i]);
        } else {
            output = valueToString(array->data.values[i]);
        }
        
        int elementLen = (int)strlen(output);
        int inc = i < len - 1 ? delimLen : 1;
        fullString = GROW_ARRAY(vm, char, fullString, strLen, strLen + elementLen + inc);
        memcpy(fullString + strLen, output, elementLen);
        strLen += elementLen;
        
        if (!IS_STRING(array->data.values[i])) {
            free(output);
        }
        
        if (i < len - 1) {
            memcpy(fullString + strLen, delim, delimLen);
            strLen += delimLen;
        }
    }
    
    fullString[strLen] = '\0';
    
    return OBJ_VAL(takeString(vm, fullString, strLen));
}

static Value arrayClear(VM *vm, int argc, const Value *args) {
    ObjArray *array = AS_ARRAY(args[0]);
    freeValueArray(vm, &array->data); // Should the array be freed or should the count be set to 0?
    initValueArray(&array->data);
    
    return ZERO_VAL;
}

static Value arrayIsEmpty(VM *vm, int argc, const Value *args) {
    ObjArray *array = AS_ARRAY(args[0]);
    return array->data.count == 0 ? TRUE_VAL : FALSE_VAL;
}

static Value arrayCopy(VM *vm, int argc, const Value *args) {
    ObjArray *array = AS_ARRAY(args[0]);
    ObjArray *ret = copyArray(vm, array, true);
    return OBJ_VAL(ret);
}

static Value arrayCopyDeep(VM *vm, int argc, const Value *args) {
    ObjArray *array = AS_ARRAY(args[0]);
    ObjArray *ret = copyArray(vm, array, false);
    return OBJ_VAL(ret);
}

static Value arrayAllTruthy(VM *vm, int argc, const Value *args) {
    if (argc > 2) {
        runtimeError(vm, "Function allTruthy() expected up to 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    ObjArray *array = AS_ARRAY(args[0]);
    int start = 0;
    int stop = array->data.count;

    if (argc > 0) {
        if (!IS_NUMBER(args[1])) {
            char *str = valueType(args[1]);
            runtimeError(vm, "Function allTruthy() expected type 'number' for first argument but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }

        start = AS_NUMBER(args[1]);

        if (start < 0 || start > stop) {
            start = 0;
        }
    }

    if (argc == 2) {
        if (!IS_NUMBER(args[2])) {
            char *str = valueType(args[2]);
            runtimeError(vm, "Function allTruthy() expected type 'number' for second argument but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }

        stop = AS_NUMBER(args[2]);

        if (stop < start || stop > array->data.count) {
            stop = array->data.count;
        }
    }

    for (int i = start; i < stop; ++i) {
        if (isFalsy(array->data.values[i])) {
            return FALSE_VAL;
        }
    }

    return TRUE_VAL;
}

static Value arrayNoneTruthy(VM *vm, int argc, const Value *args) {
    if (argc > 2) {
        runtimeError(vm, "Function noneTruthy() expected up to 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    ObjArray *array = AS_ARRAY(args[0]);
    int start = 0;
    int stop = array->data.count;

    if (argc > 0) {
        if (!IS_NUMBER(args[1])) {
            char *str = valueType(args[1]);
            runtimeError(vm, "Function noneTruthy() expected type 'number' for first argument but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }

        start = AS_NUMBER(args[1]);

        if (start < 0 || start > stop) {
            start = 0;
        }
    }

    if (argc == 2) {
        if (!IS_NUMBER(args[2])) {
            char *str = valueType(args[2]);
            runtimeError(vm, "Function noneTruthy() expected type 'number' for second argument but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }

        stop = AS_NUMBER(args[2]);

        if (stop < start || stop > array->data.count) {
            stop = array->data.count;
        }
    }

    for (int i = start; i < stop; ++i) {
        if (!isFalsy(array->data.values[i])) {
            return FALSE_VAL;
        }
    }

    return TRUE_VAL;
}

static Value arrayAnyTruthy(VM *vm, int argc, const Value *args) {
    if (argc > 2) {
        runtimeError(vm, "Function anyTruthy() expected up to 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    ObjArray *array = AS_ARRAY(args[0]);
    int start = 0;
    int stop = array->data.count;

    if (argc > 0) {
        if (!IS_NUMBER(args[1])) {
            char *str = valueType(args[1]);
            runtimeError(vm, "Function anyTruthy() expected type 'number' for first argument but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }

        start = AS_NUMBER(args[1]);

        if (start < 0 || start > stop) {
            start = 0;
        }
    }

    if (argc == 2) {
        if (!IS_NUMBER(args[2])) {
            char *str = valueType(args[2]);
            runtimeError(vm, "Function anyTruthy() expected type 'number' for second argument but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }

        stop = AS_NUMBER(args[2]);

        if (stop < start || stop > array->data.count) {
            stop = array->data.count;
        }
    }

    for (int i = start; i < stop; ++i) {
        if (!isFalsy(array->data.values[i])) {
            return TRUE_VAL;
        }
    }

    return FALSE_VAL;
}

static Value arrayForEach(VM *vm, int argc, const Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function forEach() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_CLOSURE(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function forEach() expected type 'closure' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    ObjArray *array = AS_ARRAY(args[0]);
    ObjClosure *closure = AS_CLOSURE(args[1]);

    for (int i = 0; i < array->data.count; ++i) {
        Value value = callFromScript(vm, closure, 1, &array->data.values[i]);
        
        if (IS_ERR(value)) {
            return ERROR_VAL;
        }
    }

    return ZERO_VAL;
}

static Value arrayMap(VM *vm, int argc, const Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function map() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_CLOSURE(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function map() expected type 'closure' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    ObjArray *array = AS_ARRAY(args[0]);
    ObjClosure *closure = AS_CLOSURE(args[1]);
    ObjArray *ret = newArray(vm);

    for (int i = 0; i < array->data.count; ++i) {
        Value value = callFromScript(vm, closure, 1, &array->data.values[i]);
        
        if (IS_ERR(value)) {
            return ERROR_VAL;
        }
        
        writeValueArray(vm, &ret->data, value);
    }

    return OBJ_VAL(ret);
}

static Value arrayFilter(VM *vm, int argc, const Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function filter() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!IS_CLOSURE(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function filter() expected type 'closure' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    ObjClosure *closure = AS_CLOSURE(args[1]);
    ObjArray *ret = newArray(vm);
    
    for (int i = 0; i < array->data.count; ++i) {
        Value value = callFromScript(vm, closure, 1, &array->data.values[i]);
        
        if (IS_ERR(value)) {
            return ERROR_VAL;
        }
        
        if (!IS_BOOL(value)) {
            char *type = valueType(value);
            runtimeError(vm, "Function filter() expected return type 'bool' but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }
        
        if (AS_BOOL(value)) {
            writeValueArray(vm, &ret->data, array->data.values[i]);
        }
    }
    
    return OBJ_VAL(ret);
}

static Value arrayReduce(VM *vm, int argc, const Value *args) {
    if (argc == 0 || argc > 2) {
        runtimeError(vm, "Function reduce() expected 1 or 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!IS_CLOSURE(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function reduce() expected type 'closure' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }
    
    ObjArray *array = AS_ARRAY(args[0]);
    ObjClosure *closure = AS_CLOSURE(args[1]);
    Value accumulator = ZERO_VAL;
    
    if (argc == 2) {
        accumulator = args[2];
    }
    
    Value *argv = malloc(sizeof(Value) * 2);
    
    for (int i = 0; i < array->data.count; ++i) {
        argv[0] = accumulator;
        argv[1] = array->data.values[i];
        accumulator = callFromScript(vm, closure, 2, argv);
    }
    
    free(argv);
    return accumulator;
}

#define setUpOfFunctions(name) \
    do {\
        if (argc == 0 || argc > 3) {\
            runtimeError(vm, "Function %s() expected 1 to 3 arguments but got '%d'.", name, argc);\
            return ERROR_VAL;\
        }                      \
        \
        if (!IS_CLOSURE(args[1])) {\
            char *str = valueType(args[1]);\
            runtimeError(vm, "Function %s() expected type 'closure' for first argument but got '%s'.", name, str);\
            free(str);\
            return ERROR_VAL;\
        }\
\
        closure = AS_CLOSURE(args[1]);\
\
        if (argc > 1) {\
            if (!IS_NUMBER(args[2])) {\
                char *str = valueType(args[2]);\
                runtimeError(vm, "Function %s() expected type 'number' for second argument but got '%s'.", name, str);\
                free(str);\
                return ERROR_VAL;\
            }\
\
            start = AS_NUMBER(args[2]);\
\
            if (start < 0 || start > stop) {\
                start = 0;\
            }\
        }\
\
        if (argc == 3) {\
            if (!IS_NUMBER(args[3])) {\
                char *str = valueType(args[3]);\
                runtimeError(vm, "Function %s() expected type 'number' for third argument but got '%s'.", name, str);\
                free(str);\
                return ERROR_VAL;\
            }\
\
            stop = AS_NUMBER(args[3]);\
\
            if (stop < start || stop > array->data.count) {\
                stop = array->data.count;\
            }\
        }\
    } while (false)

static Value arrayAllOf(VM *vm, int argc, const Value *args) {
    ObjArray *array = AS_ARRAY(args[0]);
    int start = 0;
    int stop = array->data.count;
    ObjClosure *closure;

    setUpOfFunctions("allOf");

    for (int i = start; i < stop; ++i) {
        Value value = callFromScript(vm, closure, 1, &array->data.values[i]);

        if (IS_ERR(value)) {
            return ERROR_VAL;
        }

        if (!IS_BOOL(value)) {
            char *type = valueType(value);
            runtimeError(vm, "Function allOf() expected return type 'bool' but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        if (!AS_BOOL(value)) {
            return FALSE_VAL;
        }
    }

    return TRUE_VAL;
}

static Value arrayNoneOf(VM *vm, int argc, const Value *args) {
    ObjArray *array = AS_ARRAY(args[0]);
    int start = 0;
    int stop = array->data.count;
    ObjClosure *closure;

    setUpOfFunctions("noneOf");

    for (int i = start; i < stop; ++i) {
        Value value = callFromScript(vm, closure, 1, &array->data.values[i]);

        if (IS_ERR(value)) {
            return ERROR_VAL;
        }

        if (!IS_BOOL(value)) {
            char *type = valueType(value);
            runtimeError(vm, "Function noneOf() expected return type 'bool' but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        if (AS_BOOL(value)) {
            return FALSE_VAL;
        }
    }

    return TRUE_VAL;
}

static Value arrayAnyOf(VM *vm, int argc, const Value *args) {
    ObjArray *array = AS_ARRAY(args[0]);
    int start = 0;
    int stop = array->data.count;
    ObjClosure *closure;

    setUpOfFunctions("anyOf");

    for (int i = start; i < stop; ++i) {
        Value value = callFromScript(vm, closure, 1, &array->data.values[i]);

        if (IS_ERR(value)) {
            return ERROR_VAL;
        }

        if (!IS_BOOL(value)) {
            char *type = valueType(value);
            runtimeError(vm, "Function anyOf() expected return type 'bool' but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        if (AS_BOOL(value)) {
            return TRUE_VAL;
        }
    }

    return FALSE_VAL;
}

void defineArrayFunctions(VM *vm) {
    defineNative(vm, "len", arrayLen, &vm->arrayFunctions);
    defineNative(vm, "toString", arrayToStringLib, &vm->arrayFunctions);
    defineNative(vm, "push", arrayPush, &vm->arrayFunctions);
    defineNative(vm, "make", arrayMake, &vm->arrayFunctions);
    defineNative(vm, "fill", arrayFill, &vm->arrayFunctions);
    defineNative(vm, "pop", arrayPop, &vm->arrayFunctions);
    defineNative(vm, "insert", arrayInsert, &vm->arrayFunctions);
    defineNative(vm, "erase", arrayErase, &vm->arrayFunctions);
    defineNative(vm, "remove", arrayRemove, &vm->arrayFunctions);
    defineNative(vm, "contains", arrayContains, &vm->arrayFunctions);
    defineNative(vm, "indexOf", arrayIndexOf, &vm->arrayFunctions);
    defineNative(vm, "reverse", arrayReverse, &vm->arrayFunctions);
    defineNative(vm, "sort", arraySort, &vm->arrayFunctions);
    defineNative(vm, "join", arrayJoin, &vm->arrayFunctions);
    defineNative(vm, "clear", arrayClear, &vm->arrayFunctions);
    defineNative(vm, "isEmpty", arrayIsEmpty, &vm->arrayFunctions);
    defineNative(vm, "shallowCopy", arrayCopy, &vm->arrayFunctions);
    defineNative(vm, "deepCopy", arrayCopyDeep, &vm->arrayFunctions);
    defineNative(vm, "allTruthy", arrayAllTruthy, &vm->arrayFunctions);
    defineNative(vm, "noneTruthy", arrayNoneTruthy, &vm->arrayFunctions);
    defineNative(vm, "anyTruthy", arrayAnyTruthy, &vm->arrayFunctions);
    // extend
    // splice
    // toSpliced
    // slice

    defineNative(vm, "forEach", arrayForEach, &vm->arrayFunctions);
    defineNative(vm, "map", arrayMap, &vm->arrayFunctions);
    defineNative(vm, "filter", arrayFilter, &vm->arrayFunctions);
    defineNative(vm, "reduce", arrayReduce, &vm->arrayFunctions);
    defineNative(vm, "allOf", arrayAllOf, &vm->arrayFunctions);
    defineNative(vm, "noneOf", arrayNoneOf, &vm->arrayFunctions);
    defineNative(vm, "anyOf", arrayAnyOf, &vm->arrayFunctions);
}