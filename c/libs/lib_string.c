//
// Created by Skyler on 4/21/22.
//

#include "lib_string.h"
#include "../memory.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

char *trimWhitespace(size_t *retLen, size_t len, const char *str) {
    char *ret = NULL;

    if (retLen == NULL) {
        return NULL;
    }

    if (len == 0) {
        ret = (char*)malloc(sizeof(char));
        *ret = '\0';
        *retLen = 0;
        return ret;
    }

    const char *end;

    while (isspace(*str)) {
        ++str;
    }

    if (*str == 0) {
        ret = (char*)malloc(sizeof(char));
        *ret = '\0';
        *retLen = 0;
        return ret;
    }

    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        --end;
    }

    end++;
    *retLen = (end - str) < len - 1 ? (end - str) : len - 1;

    ret = (char*)malloc(sizeof(char) * *retLen);
    memcpy(ret, str, *retLen);
    ret[*retLen] = '\0';

    return ret;
}

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
        return ERROR_VAL;
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

static Value stringToNumber(VM *vm, int argc, Value *args) {
    if (argc != 0) {
        runtimeError(vm, "Function toNumber() expected 0 arguments but got %d.", argc);
        return ERROR_VAL;
    }

    char *numberString = AS_CSTRING(args[0]);
    size_t numberStringLen = strlen(numberString);
    char *end;
    errno = 0;

    char *trimedNumberStr = NULL;
    size_t unused = 0;
    trimedNumberStr = trimWhitespace(&unused, numberStringLen, numberString);
    double number = strtod(trimedNumberStr, &end);

    // Failed conversion
    if (errno != 0 || *end != '\0') {
        free(trimedNumberStr);
        return NULL_VAL;
    }

    free(trimedNumberStr);
    return NUMBER_VAL(number);
}

void defineStringFunctions(VM *vm) {
    defineNative(vm, "toUpper", stringToUpper, &vm->stringFunctions);
    defineNative(vm, "toLower", stringToLower, &vm->stringFunctions);
    defineNative(vm, "len", stringLen, &vm->stringFunctions);
    defineNative(vm, "contains", stringContains, &vm->stringFunctions);
    defineNative(vm, "toNumber", stringToNumber, &vm->stringFunctions);
}