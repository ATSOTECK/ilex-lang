//
// Created by Skyler on 4/17/22.
//

#include <stdio.h>
#include <time.h>
#include <string.h>
#include "lib_natives.h"

static Value println(int argc, Value *args) {
    for (int i = 0; i < argc; ++i) {
        printValue(args[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }

    printf("\n");

    return NUMBER_VAL(0);
}

static Value ln(int argc, Value *args) {
    int count = 1;

    if (argc > 1) {
        runtimeError("Function ln() expected 1 or 0 arguments but got %d", argc);
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

static Value print(int argc, Value *args) {
    for (int i = 0; i < argc; ++i) {
        printValue(args[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }

    return NUMBER_VAL(0);
}

static Value typeof_(int argc, Value *args) {
    if (argc != 1) {
        runtimeError("Function typeof() expected 1 argument but got %d.", argc);
        return NULL_VAL;
    }

    char* type = valueType(args[0]);
    return OBJ_VAL(takeString(type, strlen(type)));
}

static Value seconds(int argc, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value milliseconds(int argc, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC * 1000);
}

static Value ilexVersionString(int argc, Value *args) {
    return OBJ_VAL(takeString(newCString(ILEX_VERSION), strlen(ILEX_VERSION)));
}

static Value ilexVersionMajor(int argc, Value *args) {
    return NUMBER_VAL(ILEX_VERSION_MAJOR);
}

static Value ilexVersionMinor(int argc, Value *args) {
    return NUMBER_VAL(ILEX_VERSION_MINOR);
}

void defineNatives(VM *_vm) {
    defineNative("println", println, &_vm->globals);
    defineNative("debugln", println, &_vm->globals); // Same as println but more searchable.
    defineNative("ln", ln, &_vm->globals);
    defineNative("print", print, &_vm->globals);
    defineNative("debug", print, &_vm->globals); // Same as print but more searchable.
    defineNative("typeof", typeof_, &_vm->globals);
    defineNative("seconds", seconds, &_vm->globals);
    defineNative("milliseconds", milliseconds, &_vm->globals);

    defineNative("ilexVersion", ilexVersionString, &_vm->globals);
    defineNative("ilexVersionMajor", ilexVersionMajor, &_vm->globals);
    defineNative("ilexVersionMinor", ilexVersionMinor, &_vm->globals);
}