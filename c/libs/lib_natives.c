//
// Created by Skyler on 4/17/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "lib_natives.h"

static Value flushConsole(VM *vm, int argc, Value *args) {
    fflush(stdout);
    return NUMBER_VAL(0);
}

static Value println(VM *vm, int argc, Value *args) {
    for (int i = 0; i < argc; ++i) {
        printValue(args[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }

    printf("\n");

    return NUMBER_VAL(0);
}

static Value ln(VM *vm, int argc, Value *args) {
    int count = 1;

    if (argc > 1) {
        runtimeError(vm, "Function ln() expected 1 or 0 arguments but got %d", argc);
    } else if (argc == 1) {
        count = (int)AS_NUMBER(args[0]);
        if (count <= 0) {
            count = 1;
        }
    }

    for (int i = 0; i < count; ++i) {
        printf("\n");
    }

    return NUMBER_VAL(0);
}

static Value print(VM *vm, int argc, Value *args) {
    for (int i = 0; i < argc; ++i) {
        printValue(args[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }

    return NUMBER_VAL(0);
}

static Value stdErr(VM *vm, int argc, Value *args) {
    for (int i = 0; i < argc; ++i) {
        char *str = valueToString(args[i]);
        fprintf(stderr, "%s", str);
        free(str);
        if (i < argc - 1) {
            fprintf(stderr, " ");
        }
    }
    
    fprintf(stderr, "\n");
    
    return NUMBER_VAL(0);
}

static Value typeof_(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function typeof() expected 1 argument but got %d.", argc);
        return NULL_VAL;
    }

    char* type = valueType(args[0]);
    return OBJ_VAL(takeString(vm, type, strlen(type)));
}

// TODO: Broken on windows. Must fix.
static Value seconds(VM *vm, int argc, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

// TODO: Broken on windows. Must fix.
static Value milliseconds(VM *vm, int argc, Value* args) {
#ifndef I_WIN
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC * 1000);
#else
    return NUMBER_VAL((double)clock());
#endif
}

void defineNatives(VM *vm) {
    defineNative(vm, "flushConsole", flushConsole, &vm->globals);
    defineNative(vm, "println", println, &vm->globals);
    defineNative(vm, "debugln", println, &vm->globals); // Same as println but more searchable.
    defineNative(vm, "ln", ln, &vm->globals);
    defineNative(vm, "print", print, &vm->globals);
    defineNative(vm, "debug", print, &vm->globals); // Same as print but more searchable.
    defineNative(vm, "printErr", stdErr, &vm->globals);
    defineNative(vm, "typeof", typeof_, &vm->globals);

    // Move these into Ilex library?
    defineNative(vm, "seconds", seconds, &vm->globals);
    defineNative(vm, "milliseconds", milliseconds, &vm->globals);
}