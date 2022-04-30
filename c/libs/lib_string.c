//
// Created by Skyler on 4/21/22.
//

#include "lib_string.h"
#include "../memory.h"

#include <ctype.h>
#include <stdlib.h>

static Value stringToUpper(VM *vm, int argc, Value *args) {
    ObjString *string = AS_STRING(args[0]);
    char *tmp = ALLOCATE(vm, char, string->len + 1);

    for (int i = 0; string->str[i]; i++) {
        tmp[i] = (char)toupper(string->str[i]);
    }
    tmp[string->len] = '\0';

    return OBJ_VAL(takeString(vm, tmp, string->len));
}

static Value stringToLower(VM *vm, int argc, Value *args) {
    ObjString *string = AS_STRING(args[0]);
    char *tmp = ALLOCATE(vm, char, string->len + 1);

    for (int i = 0; string->str[i]; i++) {
        tmp[i] = (char)tolower(string->str[i]);
    }
    tmp[string->len] = '\0';

    return OBJ_VAL(takeString(vm, tmp, string->len));
}

static Value stringLen(VM *vm, int argc, Value *args) {
    ObjString *string = AS_STRING(args[0]);
    return NUMBER_VAL(string->len);
}

static Value stringContains(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function contains() expected 1 argument but got %d", argc);
        return EMPTY_VAL;
    }

    if (!IS_STRING(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function contains() expected type 'string' but got '%s'.", str);
        free(str);
    }

    char *string = AS_CSTRING(args[0]);
    char *toFind = AS_CSTRING(args[1]);

    return BOOL_VAL(strstr(string, toFind) != NULL);
}

void defineStringFunctions(VM *vm) {
    defineNative(vm, "toUpper", stringToUpper, &vm->stringFunctions);
    defineNative(vm, "toLower", stringToLower, &vm->stringFunctions);
    defineNative(vm, "len", stringLen, &vm->stringFunctions);
    defineNative(vm, "contains", stringContains, &vm->stringFunctions);
}