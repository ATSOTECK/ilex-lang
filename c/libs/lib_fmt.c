//
// Created by Skyler Burwell on 3/3/25.
//

#include "lib_fmt.h"

#include <stdlib.h>

static Value fmtPrintColors(VM *vm, const int argc, const Value *args) {
    const int r = rand() % 2;
    if (r == 0) {
        for (int i = 0; i < 256; ++i) {
            printf("\033[38;5;%dmThe quick brown fox jumped over the lazy dog. %d\n", i, i);
        }
    } else if (r == 1) {
        for (int i = 0; i < 256; ++i) {
            printf("\033[38;5;%dmBlack sphinx of quartz judge my vow. %d\n", i, i);
        }
    }
    printf("\033[0m");

    return NULL_VAL;
}

static Value fmtColor(VM *vm, const int argc, const Value *args) {
    if (argc < 1 || argc > 3) {
        runtimeError(vm, "Function color() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_NUMBER(args[0])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Function color() expected type 'number' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    const int c = AS_NUMBER(args[0]);
    printf("\033[38;5;%dm", c);

    return OBJ_VAL(copyString(vm, "", 0));
}

Value useFmtLib(VM *vm) {
    ObjString *name = copyString(vm, "fmt", 3);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    if (lib->used) {
        return OBJ_VAL(lib);
    }

    defineNative(vm, "printColors", fmtPrintColors, &lib->values);
    defineNative(vm, "color", fmtColor, &lib->values);

    defineNativeValue(vm, "reset", OBJ_VAL(copyString(vm, "\033[0m", 4)), &lib->values);
    defineNativeValue(vm, "black", OBJ_VAL(copyString(vm, "\033[30m", 5)), &lib->values);
    defineNativeValue(vm, "red", OBJ_VAL(copyString(vm, "\033[31m", 5)), &lib->values);
    defineNativeValue(vm, "green", OBJ_VAL(copyString(vm, "\033[32m", 5)), &lib->values);
    defineNativeValue(vm, "yellow", OBJ_VAL(copyString(vm, "\033[33m", 5)), &lib->values);
    defineNativeValue(vm, "blue", OBJ_VAL(copyString(vm, "\033[34m", 5)), &lib->values);
    defineNativeValue(vm, "magenta", OBJ_VAL(copyString(vm, "\033[35m", 5)), &lib->values);
    defineNativeValue(vm, "cyan", OBJ_VAL(copyString(vm, "\033[36m", 5)), &lib->values);
    defineNativeValue(vm, "lightGray", OBJ_VAL(copyString(vm, "\033[37m", 5)), &lib->values);
    defineNativeValue(vm, "gray", OBJ_VAL(copyString(vm, "\033[90m", 5)), &lib->values);
    defineNativeValue(vm, "brightRed", OBJ_VAL(copyString(vm, "\033[91m", 5)), &lib->values);
    defineNativeValue(vm, "brightGreen", OBJ_VAL(copyString(vm, "\033[92m", 5)), &lib->values);
    defineNativeValue(vm, "brightYellow", OBJ_VAL(copyString(vm, "\033[93m", 5)), &lib->values);
    defineNativeValue(vm, "brightBlue", OBJ_VAL(copyString(vm, "\033[94m", 5)), &lib->values);
    defineNativeValue(vm, "brightMagenta", OBJ_VAL(copyString(vm, "\033[95m", 5)), &lib->values);
    defineNativeValue(vm, "brightCyan", OBJ_VAL(copyString(vm, "\033[96m", 5)), &lib->values);
    defineNativeValue(vm, "white", OBJ_VAL(copyString(vm, "\033[97m", 5)), &lib->values);


    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}
