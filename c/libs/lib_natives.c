//
// Created by Skyler on 4/17/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "lib_natives.h"

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

static Value ilexVersionString(VM *vm, int argc, Value *args) {
    return OBJ_VAL(takeString(vm, newCString(ILEX_VERSION), strlen(ILEX_VERSION)));
}

static Value ilexVersionMajor(VM *vm, int argc, Value *args) {
    return NUMBER_VAL(ILEX_VERSION_MAJOR);
}

static Value ilexVersionMinor(VM *vm, int argc, Value *args) {
    return NUMBER_VAL(ILEX_VERSION_MINOR);
}

static Value ilexVersionBuild(VM *vm, int argc, Value *args) {
    return NUMBER_VAL(ILEX_VERSION_BUILD);
}

static Value ilexMemUsed(VM *vm, int argc, Value *args) {
    return NUMBER_VAL(vm->bytesAllocated);
}

static Value ilexPrintMemUsed(VM *vm, int argc, Value *args) {
    double bytes = (double)vm->bytesAllocated;
    
    int times = 0;
    while (bytes > 1000) {
        bytes /= 1000;
        ++times;
    }
    
    switch (times) {
        case 0: printf("%d bytes\n", (int)bytes); break;
        case 1: printf("%.15g kb\n", bytes); break;
        case 2: printf("%.15g mb\n", bytes); break;
        case 3: printf("%.15g gb\n", bytes); break;
        case 4: printf("%.15g tb\n", bytes); break;
        default: printf(">= 1000tb\n"); break;
    }
    
    return NUMBER_VAL(0);
}

static Value ilexGetMemUsed(VM *vm, int argc, Value *args) {
    double bytes = (double)vm->bytesAllocated;
    
    int times = 0;
    while (bytes > 1000) {
        bytes /= 1000;
        ++times;
    }
    
    int len = snprintf(NULL, 0, "%.15g", bytes) + 4;
    if (times == 0) {
        len += 3;
    }
    
    char *str = (char*)malloc(sizeof(char) * len);
    
    switch (times) {
        case 0: snprintf(str, len, "%d bytes", (int)bytes); break;
        case 1: snprintf(str, len, "%.15g kb", bytes); break;
        case 2: snprintf(str, len, "%.15g mb", bytes); break;
        case 3: snprintf(str, len, "%.15g gb", bytes); break;
        case 4: snprintf(str, len, "%.15g tb", bytes); break;
        default: snprintf(str, len, "???"); break;
    }
    
    ObjString *ret = takeString(vm, str, len);
    return OBJ_VAL(ret);
}

void defineNatives(VM *vm) {
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
    
    defineNative(vm, "ilexVersion", ilexVersionString, &vm->globals);
    defineNative(vm, "ilexVersionMajor", ilexVersionMajor, &vm->globals);
    defineNative(vm, "ilexVersionMinor", ilexVersionMinor, &vm->globals);
    defineNative(vm, "ilexVersionBuild", ilexVersionBuild, &vm->globals);
    
    defineNative(vm, "memAllocated", ilexMemUsed, &vm->globals);
    defineNative(vm, "memUsagePrint", ilexPrintMemUsed, &vm->globals);
    defineNative(vm, "memUsageGet", ilexGetMemUsed, &vm->globals);
}