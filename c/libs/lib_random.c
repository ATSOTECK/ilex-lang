//
// Created by Skyler on 5/4/22.
//

#include "lib_random.h"

#include "../vm.h"

#include <stdlib.h>
#include <time.h>

#define argCheckNum(name) do {\
                            if (argc != 1) { \
                                runtimeError(vm, "Function %s() expected 1 argument but got %d.", name, argc); \
                                return ERROR_VAL; \
                            } \
                            if (!IS_NUMBER(args[0])) { \
                                char *str = valueType(args[0]); \
                                runtimeError(vm, "Function %s() expected type 'number' but got '%s'.", name, str); \
                                free(str); \
                                return ERROR_VAL; \
                            } \
                        } while (false)

#define argCheckNum2(name) do {\
                            if (argc != 2) { \
                                runtimeError(vm, "Function %s() expected 2 arguments but got %d.", name, argc); \
                                return ERROR_VAL; \
                            }  \
                            for (int i = 0; i < argc; ++i) {   \
                                if (!IS_NUMBER(args[i])) { \
                                    char *str = valueType(args[i]); \
                                    runtimeError(vm, "Function %s() expected type 'number' but got '%s'.", name, str); \
                                    free(str); \
                                    return ERROR_VAL; \
                                } \
                            } \
                        } while (false)

unsigned int randomLibSeed;

static Value randomSeed(VM *vm, int argc, Value *args) {
    argCheckNum("seed");

    srand((unsigned  int)AS_NUMBER(args[0]));

    return ZERO_VAL;
}

static Value randomRandomSeed(VM *vm, int argc, Value *args) {
    argCheckNum("randomSeed");

    srand((unsigned  int)rand() % RAND_MAX);

    return ZERO_VAL;
}

static Value randomGetSeed(VM *vm, int argc, Value *args) {
    return NUMBER_VAL(randomLibSeed);
}

static Value randomRand(VM *vm, int argc, Value *args) {
    return NUMBER_VAL((double)rand() / RAND_MAX);
}

static Value randomRandom(VM *vm, int argc, Value *args) {
    if (argc == 0) {
        return NUMBER_VAL((double)rand());
    }

    if (!IS_NUMBER(args[0])) {
        char *str = valueType(args[0]);
        runtimeError(vm, "Function number() expected type 'number' but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    return AS_NUMBER((double)rand() / (double)(RAND_MAX / AS_NUMBER(args[0])));
}

static Value randomRandomRange(VM *vm, int argc, Value *args) {
    argCheckNum2("randomRange");

    return AS_NUMBER((double)rand() / (double)(RAND_MAX / AS_NUMBER(args[0]) - AS_NUMBER(args[1])));
}

static Value randomRandomI(VM *vm, int argc, Value *args) {
    if (argc == 0) {
        return NUMBER_VAL(rand());
    }

    if (!IS_NUMBER(args[0])) {
        char *str = valueType(args[0]);
        runtimeError(vm, "Function int() expected type 'number' but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    return AS_NUMBER(rand() % (unsigned int)AS_NUMBER(args[0]));
}

static Value randomRandomRangeI(VM *vm, int argc, Value *args) {
    argCheckNum2("intRange");

    return AS_NUMBER(rand() % (unsigned int)( AS_NUMBER(args[0]) - AS_NUMBER(args[1])));
}

static Value randomChoose(VM *vm, int argc, Value *args) {
    if (argc == 0) {
        return NULL_VAL;
    }
    
    if (argc == 1 && IS_ARRAY(args[0])) {
        ObjArray *array = AS_ARRAY(args[0]);
        int len = array->data.count;
        
        if (len == 0) {
            return NULL_VAL;
        } else if (len == 1) {
            return array->data.values[0];
        }
        
        return array->data.values[rand() % len];
    } else if (argc == 1) {
        return args[0];
    }
    
    return args[rand() % argc];
}

Value useRandomLib(VM *vm) {
    ObjString *name = copyString(vm, "random", 6);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));
    
    if (lib->used) {
        return OBJ_VAL(lib);
    }

    randomLibSeed = time(NULL);
    srand(randomLibSeed);

    defineNative(vm, "seed", randomSeed, &lib->values);
    defineNative(vm, "randomSeed", randomRandomSeed, &lib->values);
    defineNative(vm, "getSeed", randomGetSeed, &lib->values);
    defineNative(vm, "rand", randomRand, &lib->values);
    defineNative(vm, "number", randomRandom, &lib->values);
    defineNative(vm, "numberRange", randomRandomRange, &lib->values);
    defineNative(vm, "int", randomRandomI, &lib->values);
    defineNative(vm, "intRange", randomRandomRangeI, &lib->values);
    defineNative(vm, "choose", randomChoose, &lib->values);

#undef argCheckNum
#undef argCheckNum2

    defineNativeValue(vm, "RAND_MAX", NUMBER_VAL(RAND_MAX), &lib->values);

    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}