//
// Created by Skyler on 5/1/22.
//

#include "lib_ilex.h"

#include "../vm.h"
#include "../memory.h"

#include <stdio.h>
#include <stdlib.h>

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

static Value ilexDateString(VM *vm, int argc, Value *args) {
    return OBJ_VAL(takeString(vm, newCString(ILEX_DATE), strlen(ILEX_DATE)));
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

    return ZERO_VAL;
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

static Value ilexNextGcAt(VM *vm, int argc, Value *args) {
    double bytes = (double)vm->nextGC;

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

static Value ilexGcRuns(VM *vm, int argc, Value *args) {
    return NUMBER_VAL((double)vm->gcRuns);
}

static Value ilexCollectGarbage(VM *vm, int argc, Value *args) {
    collectGarbage(vm);
    return ZERO_VAL;
}

static Value ilexFunctionCount(VM *vm, int argc, Value *args) {
    return NUMBER_VAL(vm->fnCount);
}

static Value ilexValueCount(VM *vm, int argc, Value *args) {
    return NUMBER_VAL(vm->valCount);
}

Value useIlexLib(VM *vm) {
    ObjString *name = copyString(vm, "ilex", 4);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));
    
    if (lib->used) {
        return OBJ_VAL(lib);
    }

    defineNative(vm, "version", ilexVersionString, &lib->values);
    defineNative(vm, "versionMajor", ilexVersionMajor, &lib->values);
    defineNative(vm, "versionMinor", ilexVersionMinor, &lib->values);
    defineNative(vm, "versionBuild", ilexVersionBuild, &lib->values);
    defineNative(vm, "versionDate", ilexDateString, &lib->values);

    defineNative(vm, "memAllocated", ilexMemUsed, &lib->values);
    defineNative(vm, "printMemUsage", ilexPrintMemUsed, &lib->values);
    defineNative(vm, "getMemUsage", ilexGetMemUsed, &lib->values);
    defineNative(vm, "nextGC", ilexNextGcAt, &lib->values);
    defineNative(vm, "gcRuns", ilexGcRuns, &lib->values);

    defineNative(vm, "collectGarbage", ilexCollectGarbage, &lib->values);

    defineNative(vm, "functionCount", ilexFunctionCount, &lib->values);
    defineNative(vm, "valueCount", ilexValueCount, &lib->values);
    
    defineNativeValue(vm, "argc", NUMBER_VAL(vm->argc), &lib->values);
    ObjArray *arr = newArray(vm);
    push(vm, OBJ_VAL(arr));
    
    for (int i = 0; i < vm->argc; ++i) {
        Value arg = OBJ_VAL(copyString(vm, vm->argv[i], strlen(vm->argv[i])));
        push(vm, arg);
        writeValueArray(vm, &arr->data, arg);
        pop(vm);
    }
    
    defineNativeValue(vm, "args", OBJ_VAL(arr), &lib->values);

    pop(vm);
    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}