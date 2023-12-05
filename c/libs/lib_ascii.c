//
// Created by Skyler on 12/1/23.
//

#include "lib_ascii.h"

#include "../vm.h"

#include <stdlib.h>

static Value asciiIsAlpha(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function isAlpha() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function isAlpha() expected type 'string' for first argument but got '%s'.", type);
        free(type);

        return ERROR_VAL;
    }

    char c = AS_CSTRING(args[0])[0];
    return BOOL_VAL((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

static Value asciiIsDigit(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function isDigit() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function isDigit() expected type 'string' for first argument but got '%s'.", type);
        free(type);

        return ERROR_VAL;
    }

    char c = AS_CSTRING(args[0])[0];
    return BOOL_VAL(c >= '0' && c <= '9');
}

static Value asciiIsAlphanumeric(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function isAlphanumeric() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function isAlphanumeric() expected type 'string' for first argument but got '%s'.", type);
        free(type);

        return ERROR_VAL;
    }

    char c = AS_CSTRING(args[0])[0];
    return BOOL_VAL((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'));
}

Value useAsciiLib(VM *vm) {
    ObjString *name = copyString(vm, "ascii", 5);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    if (lib->used) {
        return OBJ_VAL(lib);
    }

    defineNative(vm, "isAlpha", asciiIsAlpha, &lib->values);
    defineNative(vm, "isDigit", asciiIsDigit, &lib->values);
    defineNative(vm, "isAlphanumeric", asciiIsAlphanumeric, &lib->values);

    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}
