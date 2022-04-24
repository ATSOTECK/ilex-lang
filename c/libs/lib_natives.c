//
// Created by Skyler on 4/17/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "lib_natives.h"

static Value println(VM *_vm, int argc, Value *args) {
    for (int i = 0; i < argc; ++i) {
        printValue(args[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }

    printf("\n");

    return NUMBER_VAL(0);
}

static Value ln(VM *_vm, int argc, Value *args) {
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

static Value print(VM *_vm, int argc, Value *args) {
    for (int i = 0; i < argc; ++i) {
        printValue(args[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }

    return NUMBER_VAL(0);
}

static Value stdErr(VM *_vm, int argc, Value *args) {
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

static Value typeof_(VM *_vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError("Function typeof() expected 1 argument but got %d.", argc);
        return NULL_VAL;
    }

    char* type = valueType(args[0]);
    return OBJ_VAL(takeString(type, strlen(type)));
}

static Value seconds(VM *_vm, int argc, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value milliseconds(VM *_vm, int argc, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC * 1000);
}

static Value ilexVersionString(VM *_vm, int argc, Value *args) {
    return OBJ_VAL(takeString(newCString(ILEX_VERSION), strlen(ILEX_VERSION)));
}

static Value ilexVersionMajor(VM *_vm, int argc, Value *args) {
    return NUMBER_VAL(ILEX_VERSION_MAJOR);
}

static Value ilexVersionMinor(VM *_vm, int argc, Value *args) {
    return NUMBER_VAL(ILEX_VERSION_MINOR);
}

static Value ilexVersionBuild(VM *_vm, int argc, Value *args) {
    return NUMBER_VAL(ILEX_VERSION_BUILD);
}

static Value ilexMemUsed(VM *_vm, int argc, Value *args) {
    return NUMBER_VAL(_vm->bytesAllocated);
}

static Value ilexPrintMemUsed(VM *_vm, int argc, Value *args) {
    double bytes = _vm->bytesAllocated;
    
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

static Value ilexGetMemUsed(VM *_vm, int argc, Value *args) {
    double bytes = _vm->bytesAllocated;
    
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
    
    ObjString *ret = takeString(str, len);
    return OBJ_VAL(ret);
}

void defineNatives(VM *_vm) {
    defineNative("println", println, &_vm->globals);
    defineNative("debugln", println, &_vm->globals); // Same as println but more searchable.
    defineNative("ln", ln, &_vm->globals);
    defineNative("print", print, &_vm->globals);
    defineNative("debug", print, &_vm->globals); // Same as print but more searchable.
    defineNative("printErr", stdErr, &_vm->globals);
    defineNative("typeof", typeof_, &_vm->globals);
    
    // Move these into Ilex library?
    defineNative("seconds", seconds, &_vm->globals);
    defineNative("milliseconds", milliseconds, &_vm->globals);
    
    defineNative("ilexVersion", ilexVersionString, &_vm->globals);
    defineNative("ilexVersionMajor", ilexVersionMajor, &_vm->globals);
    defineNative("ilexVersionMinor", ilexVersionMinor, &_vm->globals);
    defineNative("ilexVersionBuild", ilexVersionBuild, &_vm->globals);
    
    defineNative("memAllocated", ilexMemUsed, &_vm->globals);
    defineNative("memUsagePrint", ilexPrintMemUsed, &_vm->globals);
    defineNative("memUsageGet", ilexGetMemUsed, &_vm->globals);
}