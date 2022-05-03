//
// Created by Skyler on 4/29/22.
//

#include "lib_math.h"

#include "../vm.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

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

static Value mathToRad(VM *vm, int argc, Value *args) {
    argCheckNum("toRad");

    return NUMBER_VAL(0.01745329251994329576923690768489 * AS_NUMBER(args[0]));
}

static Value mathToDeg(VM *vm, int argc, Value *args) {
    argCheckNum("toRad");

    return NUMBER_VAL(57.295779513082320876798154814105 * AS_NUMBER(args[0]));
}

static Value mathRound(VM *vm, int argc, Value *args) {
    argCheckNum("round");

    return NUMBER_VAL(round(AS_NUMBER(args[0])));
}

static Value mathSqrt(VM *vm, int argc, Value *args) {
    argCheckNum("sqrt");

    return NUMBER_VAL(sqrt(AS_NUMBER(args[0])));
}

static Value mathSin(VM *vm, int argc, Value *args) {
    argCheckNum("sin");

    return NUMBER_VAL(sin(AS_NUMBER(args[0])));
}

static Value mathCos(VM *vm, int argc, Value *args) {
    argCheckNum("cos");

    return NUMBER_VAL(cos(AS_NUMBER(args[0])));
}

static Value mathTan(VM *vm, int argc, Value *args) {
    argCheckNum("tan");

    return NUMBER_VAL(tan(AS_NUMBER(args[0])));
}

static Value mathASin(VM *vm, int argc, Value *args) {
    argCheckNum("asin");

    return NUMBER_VAL(asin(AS_NUMBER(args[0])));
}

static Value mathACos(VM *vm, int argc, Value *args) {
    argCheckNum("acos");

    return NUMBER_VAL(acos(AS_NUMBER(args[0])));
}

static Value mathATan(VM *vm, int argc, Value *args) {
    argCheckNum("atan");

    return NUMBER_VAL(atan(AS_NUMBER(args[0])));
}

static Value mathSinh(VM *vm, int argc, Value *args) {
    argCheckNum("sinh");

    return NUMBER_VAL(sinh(AS_NUMBER(args[0])));
}

static Value mathCosh(VM *vm, int argc, Value *args) {
    argCheckNum("cosh");

    return NUMBER_VAL(cosh(AS_NUMBER(args[0])));
}

static Value mathTanh(VM *vm, int argc, Value *args) {
    argCheckNum("tanh");

    return NUMBER_VAL(tanh(AS_NUMBER(args[0])));
}

static Value mathExp(VM *vm, int argc, Value *args) {
    argCheckNum("exp");

    return NUMBER_VAL(exp(AS_NUMBER(args[0])));
}

static Value mathLog(VM *vm, int argc, Value *args) {
    argCheckNum("log");

    return NUMBER_VAL(log(AS_NUMBER(args[0])));
}

static Value mathLog10(VM *vm, int argc, Value *args) {
    argCheckNum("log10");

    return NUMBER_VAL(log10(AS_NUMBER(args[0])));
}

static Value mathCeil(VM *vm, int argc, Value *args) {
    argCheckNum("ceil");

    return NUMBER_VAL(ceil(AS_NUMBER(args[0])));
}

static Value mathFloor(VM *vm, int argc, Value *args) {
    argCheckNum("floor");

    return NUMBER_VAL(floor(AS_NUMBER(args[0])));
}

static Value mathAbs(VM *vm, int argc, Value *args) {
    argCheckNum("abs");

    return NUMBER_VAL(fabs(AS_NUMBER(args[0])));
}

static Value mathATan2(VM *vm, int argc, Value *args) {
    argCheckNum2("atan2");

    return NUMBER_VAL(atan2(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

/*
static Value mathFrexp(VM *vm, int argc, Value *args) {
    argCheckNum("frexp");

    //TODO(Skyler): Multiple return values.
    int exponent;
    return NUMBER_VAL(frexp(AS_NUMBER(args[0]), &exponent));
}
*/

static Value mathLdexp(VM *vm, int argc, Value *args) {
    argCheckNum2("ldexp");

    return NUMBER_VAL(ldexp(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

//TODO(Skyler): List of numbers.
static Value mathMax(VM *vm, int argc, Value *args) {
    if (argc == 0) {
        return NUMBER_VAL(0);
    }

    if (!IS_NUMBER(args[0])) {
        char *str = valueType(args[0]);
        runtimeError(vm, "Function max() expected type 'number' but got '%s'.", str);
        free(str);
        return NULL_VAL;
    }

    double max = AS_NUMBER(args[0]);

    for (int i = 1; i < argc; ++i) {
        if (!IS_NUMBER(args[i])) {
            char *str = valueType(args[i]);
            runtimeError(vm, "Function max() expected type 'number' but got '%s'.", str);
            free(str);
            return NULL_VAL;
        }

        double num = AS_NUMBER(args[i]);
        if (num > max) {
            max = num;
        }
    }

    return NUMBER_VAL(max);
}

//TODO(Skyler): List of numbers.
static Value mathMin(VM *vm, int argc, Value *args) {
    if (argc == 0) {
        return NUMBER_VAL(0);
    }

    if (!IS_NUMBER(args[0])) {
        char *str = valueType(args[0]);
        runtimeError(vm, "Function max() expected type 'number' but got '%s'.", str);
        free(str);
        return NULL_VAL;
    }

    double min = AS_NUMBER(args[0]);

    for (int i = 1; i < argc; ++i) {
        if (!IS_NUMBER(args[i])) {
            char *str = valueType(args[i]);
            runtimeError(vm, "Function max() expected type 'number' but got '%s'.", str);
            free(str);
            return NULL_VAL;
        }

        double num = AS_NUMBER(args[i]);
        if (num < min) {
            min = num;
        }
    }

    return NUMBER_VAL(min);
}

static Value mathAverage(VM *vm, int argc, Value *args) {
    if (argc == 0) {
        return NUMBER_VAL(0);
    }

    double average = 0;
    for (int i = 1; i < argc; ++i) {
        if (!IS_NUMBER(args[i])) {
            char *str = valueType(args[i]);
            runtimeError(vm, "Function max() expected type 'number' but got '%s'.", str);
            free(str);
            return NULL_VAL;
        }

        average += AS_NUMBER(args[i]);
    }

    return NUMBER_VAL(average / argc);
}

Value useMathLib(VM *vm) {
    ObjString *name = copyString(vm, "math", 4);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    defineNative(vm, "toRad", mathToRad, &lib->values);
    defineNative(vm, "toDeg", mathToDeg, &lib->values);
    defineNative(vm, "round", mathRound, &lib->values);
    defineNative(vm, "sqrt", mathSqrt, &lib->values);
    defineNative(vm, "sin", mathSin, &lib->values);
    defineNative(vm, "cos", mathCos, &lib->values);
    defineNative(vm, "tan", mathTan, &lib->values);
    defineNative(vm, "asin", mathASin, &lib->values);
    defineNative(vm, "acos", mathACos, &lib->values);
    defineNative(vm, "atan", mathATan, &lib->values);
    defineNative(vm, "sinh", mathSinh, &lib->values);
    defineNative(vm, "cosh", mathCosh, &lib->values);
    defineNative(vm, "tanh", mathTanh, &lib->values);
    defineNative(vm, "exp", mathExp, &lib->values);
    defineNative(vm, "log", mathLog, &lib->values);
    defineNative(vm, "log10", mathLog10, &lib->values);
    defineNative(vm, "ceil", mathCeil, &lib->values);
    defineNative(vm, "floor", mathFloor, &lib->values);
    defineNative(vm, "abs", mathAbs, &lib->values);
    defineNative(vm, "atan2", mathATan2, &lib->values);
    defineNative(vm, "ldexp", mathLdexp, &lib->values);
    defineNative(vm, "max", mathMax, &lib->values);
    defineNative(vm, "min", mathMin, &lib->values);
    defineNative(vm, "average", mathAverage, &lib->values);

#undef argCheckNum

    defineNativeValue(vm, "pi",        NUMBER_VAL(3.14159265358979323846264338327950288), &lib->values);
    defineNativeValue(vm, "pi_2",      NUMBER_VAL(1.57079632679489), &lib->values);
    defineNativeValue(vm, "pi_4",      NUMBER_VAL(0.78539816339744), &lib->values);
    defineNativeValue(vm, "_1_pi",     NUMBER_VAL(0.31830988618379), &lib->values);
    defineNativeValue(vm, "_2_pi",     NUMBER_VAL(0.63661977236758), &lib->values);
    defineNativeValue(vm, "_2_sqrtpi", NUMBER_VAL(1.12837916709551), &lib->values);
    defineNativeValue(vm, "tau",       NUMBER_VAL(6.28318530717958), &lib->values);
    defineNativeValue(vm, "e",         NUMBER_VAL(2.71828182845905), &lib->values);
    defineNativeValue(vm, "log2e",     NUMBER_VAL(1.44269504088896), &lib->values);
    defineNativeValue(vm, "log10e",    NUMBER_VAL(0.43429448190325), &lib->values);
    defineNativeValue(vm, "phi",       NUMBER_VAL(1.61803398874989), &lib->values);
    defineNativeValue(vm, "sqrt2",     NUMBER_VAL(1.41421356237309), &lib->values);
    defineNativeValue(vm, "sqrt1_2",   NUMBER_VAL(0.70710678118654), &lib->values);
    defineNativeValue(vm, "sqrte",     NUMBER_VAL(1.61803398874989), &lib->values);
    defineNativeValue(vm, "sqrtpi",    NUMBER_VAL(1.77245385090551), &lib->values);
    defineNativeValue(vm, "sqrtphi",   NUMBER_VAL(1.27201964951406), &lib->values);
    defineNativeValue(vm, "log2",      NUMBER_VAL(0.69314718055994), &lib->values);
    defineNativeValue(vm, "log10",     NUMBER_VAL(2.30258509299404), &lib->values);

    pop(vm);
    pop(vm);

    return OBJ_VAL(lib);
}
