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
                                return NULL_VAL; \
                            } \
                            if (!IS_NUMBER(args[0])) { \
                                char *str = valueType(args[0]); \
                                runtimeError(vm, "Function %s() expected type 'number' but got '%s'.", name, str); \
                                free(str); \
                                return NULL_VAL; \
                            } \
                        } while (false)

#define argCheckNum2(name) do {\
                            if (argc != 2) { \
                                runtimeError(vm, "Function %s() expected 2 arguments but got %d.", name, argc); \
                                return NULL_VAL; \
                            }  \
                            for (int i = 0; i < argc; ++i) {   \
                                if (!IS_NUMBER(args[i])) { \
                                    char *str = valueType(args[i]); \
                                    runtimeError(vm, "Function %s() expected type 'number' but got '%s'.", name, str); \
                                    free(str); \
                                    return NULL_VAL; \
                                } \
                            } \
                        } while (false)

unsigned int randomLibSeed;

static Value randomSeed(VM *vm, int argc, Value *args) {
    argCheckNum("seed");

    srand((unsigned  int)AS_NUMBER(args[0]));

    return NUMBER_VAL(0);
}

static Value randomRandomSeed(VM *vm, int argc, Value *args) {
    argCheckNum("randomSeed");

    srand((unsigned  int)rand() % RAND_MAX);

    return NUMBER_VAL(0);
}

static Value randomGetSeed(VM *vm, int argc, Value *args) {
    return NUMBER_VAL(randomLibSeed);
}

static Value randomRandom(VM *vm, int argc, Value *args) {
    if (argc == 0) {
        return NUMBER_VAL(rand());
    }

    if (!IS_NUMBER(args[0])) {
        char *str = valueType(args[0]);
        runtimeError(vm, "Function number() expected type 'number' but got '%s'.", str);
        free(str);
        return NULL_VAL;
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
        return NULL_VAL;
    }

    return AS_NUMBER(rand() % (unsigned int)AS_NUMBER(args[0]));
}

static Value randomRandomRangeI(VM *vm, int argc, Value *args) {
    argCheckNum2("intRange");

    return AS_NUMBER(rand() % (unsigned int)( AS_NUMBER(args[0]) - AS_NUMBER(args[1])));
}

static Value randomChoose(VM *vm, int argc, Value *args) {
    return args[rand() % argc];
}

Value useRandomLib(VM *vm) {
    ObjString *name = copyString(vm, "math", 4);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    randomLibSeed = time(NULL);
    srand(randomLibSeed);

    defineNative(vm, "seed", randomSeed, &lib->values);
    defineNative(vm, "randomSeed", randomRandomSeed, &lib->values);
    defineNative(vm, "getSeed", randomGetSeed, &lib->values);
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

    return OBJ_VAL(lib);
}