//
// Created by Skyler on 4/17/22.
//

#include <stdio.h>
#include <stdlib.h>
#include "lib_natives.h"

static Value println(VM *vm, const int argc, const Value *args) {
    for (int i = 0; i < argc; ++i) {
        printValue(args[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }

    printf("\n");

    return ZERO_VAL;
}

static Value newLine(VM *vm, const int argc, const Value *args) {
    int count = 1;

    if (argc > 1) {
        runtimeError(vm, "Function newLine() expected 1 or 0 arguments but got %d", argc);
    } else if (argc == 1) {
        count = (int)AS_NUMBER(args[0]);
        if (count <= 0) {
            count = 1;
        }
    }

    for (int i = 0; i < count; ++i) {
        printf("\n");
    }

    return ZERO_VAL;
}

static Value print(VM *vm, const int argc, const Value *args) {
    for (int i = 0; i < argc; ++i) {
        printValue(args[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }

    return ZERO_VAL;
}

static Value stdErr(VM *vm, const int argc, const Value *args) {
    for (int i = 0; i < argc; ++i) {
        char *str = valueToString(args[i]);
        fprintf(stderr, "%s", str);
        free(str);
        if (i < argc - 1) {
            fprintf(stderr, " ");
        }
    }
    
    fprintf(stderr, "\n");
    
    return ZERO_VAL;
}

static Value nativeToString(VM *vm, const int argc, const Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function toString() expected 1 argument but got %d.", argc);
        return ERROR_VAL;
    }

    char *str = valueToString(args[0]);
    ObjString *ret = copyString(vm, str, (int)strlen(str));
    free(str);

    return OBJ_VAL(ret);
}

static Value nativeToBool(VM *vm, const int argc, const Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function toBool() expected 1 argument but got %d.", argc);
        return ERROR_VAL;
    }

    return BOOL_VAL(!isFalsy(args[0]));
}

void defineNatives(VM *vm) {
    defineNative(vm, "println", println, &vm->globals);
    defineNative(vm, "debugln", println, &vm->globals); // Same as println but more searchable.
    defineNative(vm, "newLine", newLine, &vm->globals);
    defineNative(vm, "print", print, &vm->globals);
    defineNative(vm, "debug", print, &vm->globals); // Same as print but more searchable.
    defineNative(vm, "printErr", stdErr, &vm->globals);
    defineNative(vm, "toString", nativeToString, &vm->globals);
    defineNative(vm, "toBool", nativeToBool, &vm->globals);
}