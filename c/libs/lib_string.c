//
// Created by Skyler on 4/21/22.
//

#include "lib_string.h"
#include "../memory.h"

#include <ctype.h>

static Value stringToUpper(VM *_vm, int argc, Value *args) {
    ObjString *string = AS_STRING(args[0]);
    char *tmp = ALLOCATE(char, string->len + 1);

    for (int i = 0; string->str[i]; i++) {
        tmp[i] = (char)toupper(string->str[i]);
    }
    tmp[string->len] = '\0';

    return OBJ_VAL(takeString(tmp, string->len));
}

static Value stringToLower(VM *_vm, int argc, Value *args) {
    ObjString *string = AS_STRING(args[0]);
    char *tmp = ALLOCATE(char, string->len + 1);

    for (int i = 0; string->str[i]; i++) {
        tmp[i] = (char)tolower(string->str[i]);
    }
    tmp[string->len] = '\0';

    return OBJ_VAL(takeString(tmp, string->len));
}

static Value stringLen(VM *_vm, int argc, Value *args) {
    ObjString *string = AS_STRING(args[0]);
    return NUMBER_VAL(string->len);
}

static Value stringContains(VM *_vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError("Function contains() expected 1 argument but got %d", argc);
        return EMPTY_VAL;
    }

    if (!IS_STRING(args[1])) {
        runtimeError("Function contains() expected type 'string' but got '%s'.", valueType(args[1]));
    }

    char *string = AS_CSTRING(args[0]);
    char *toFind = AS_CSTRING(args[1]);

    return BOOL_VAL(strstr(string, toFind) != NULL);
}

void defineStringFunctions(VM *_vm) {
    defineNative("toUpper", stringToUpper, &_vm->stringFunctions);
    defineNative("toLower", stringToLower, &_vm->stringFunctions);
    defineNative("len", stringLen, &_vm->stringFunctions);
    defineNative("contains", stringContains, &_vm->stringFunctions);
}