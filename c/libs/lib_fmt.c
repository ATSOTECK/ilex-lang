//
// Created by Skyler Burwell on 3/3/25.
//

#include "lib_fmt.h"

#include <stdlib.h>

static Value fmtPrintColors(VM *vm, const int argc, const Value *args) {
    if (argc != 0 && argc != 1) {
        runtimeError(vm, "Function printColors() expected 0 or 1 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    printf("\033[0m"); // Reset before printing colors.

    if (argc == 0) {
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
    } else {
        if (!IS_STRING(args[0])) {
            char *type = valueType(args[1]);
            runtimeError(vm, "Function printColors() expected type 'string' for the first argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        const ObjString *str = AS_STRING(args[0]);
        for (int i = 0; i < 255; ++i) {
            printf("\033[38;5;%dm%s %d\n", i, str->str, i);
        }
    }
    printf("\033[0m");

    return NULL_VAL;
}

static Value fmtColor(VM *vm, const int argc, const Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function color8() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_NUMBER(args[0])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Function color8() expected type 'number' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    const int c = AS_NUMBER(args[0]);
    printf("\033[38;5;%dm", c);

    char *fmt = (char*)malloc(sizeof(char) * 12 + 1);
    sprintf(fmt, "\033[38;5;%dm", c);
    return OBJ_VAL(takeString(vm, fmt, strlen(fmt)));
}

static Value fmtRgb(VM *vm, const int argc, const Value *args) {
    if (argc != 3) {
        runtimeError(vm, "Function rgb() expected 3 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_NUMBER(args[0])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Function rgb() expected type 'number' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    if (!IS_NUMBER(args[1])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Function rgb() expected type 'number' for the second argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    if (!IS_NUMBER(args[2])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Function rgb() expected type 'number' for the third argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    const int r = AS_NUMBER(args[0]);
    const int g = AS_NUMBER(args[1]);
    const int b = AS_NUMBER(args[2]);

    char *fmt = (char*)malloc(sizeof(char) * 20 + 1);
    sprintf(fmt, "\033[38;2;%d;%d;%dm", r, g, b);
    return OBJ_VAL(takeString(vm, fmt, strlen(fmt)));
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
    defineNative(vm, "color8", fmtColor, &lib->values);
    defineNative(vm, "rgb", fmtRgb, &lib->values);

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
    defineNativeValue(vm, "resetColor", OBJ_VAL(copyString(vm, "\033[39m", 5)), &lib->values);
    defineNativeValue(vm, "resetBgColor", OBJ_VAL(copyString(vm, "\033[49m", 5)), &lib->values);

    defineNativeValue(vm, "bold", OBJ_VAL(copyString(vm, "\033[1m", 4)), &lib->values);
    defineNativeValue(vm, "dim", OBJ_VAL(copyString(vm, "\033[2m", 4)), &lib->values);
    defineNativeValue(vm, "italic", OBJ_VAL(copyString(vm, "\033[3m", 4)), &lib->values);
    defineNativeValue(vm, "underline", OBJ_VAL(copyString(vm, "\033[4m", 4)), &lib->values);
    defineNativeValue(vm, "doubleUnderline", OBJ_VAL(copyString(vm, "\033[21m", 5)), &lib->values); // Or reset bold, depends on terminal.
    defineNativeValue(vm, "blink", OBJ_VAL(copyString(vm, "\033[5m", 4)), &lib->values);
    defineNativeValue(vm, "fastBlink", OBJ_VAL(copyString(vm, "\033[6m", 4)), &lib->values);
    defineNativeValue(vm, "inverted", OBJ_VAL(copyString(vm, "\033[7m", 4)), &lib->values);
    defineNativeValue(vm, "hidden", OBJ_VAL(copyString(vm, "\033[8m", 4)), &lib->values);
    defineNativeValue(vm, "strike", OBJ_VAL(copyString(vm, "\033[9m", 4)), &lib->values);

    defineNativeValue(vm, "resetBold", OBJ_VAL(copyString(vm, "\033[21m", 5)), &lib->values); // Or double underline, depends on terminal.
    defineNativeValue(vm, "resetIntensity", OBJ_VAL(copyString(vm, "\033[22m", 5)), &lib->values);
    defineNativeValue(vm, "resetItalic", OBJ_VAL(copyString(vm, "\033[23m", 5)), &lib->values);
    defineNativeValue(vm, "resetUnderline", OBJ_VAL(copyString(vm, "\033[24m", 5)), &lib->values);
    defineNativeValue(vm, "resetBlink", OBJ_VAL(copyString(vm, "\033[25m", 5)), &lib->values);
    defineNativeValue(vm, "resetInverted", OBJ_VAL(copyString(vm, "\033[27m", 5)), &lib->values);
    defineNativeValue(vm, "resetHidden", OBJ_VAL(copyString(vm, "\033[28m", 5)), &lib->values);
    defineNativeValue(vm, "resetStrike", OBJ_VAL(copyString(vm, "\033[29m", 5)), &lib->values);


    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}
