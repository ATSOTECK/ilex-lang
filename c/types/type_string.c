//
// Created by Skyler on 4/21/22.
//

#include "type_string.h"
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

    ret = (char*)malloc(sizeof(char) * *retLen + 1);
    memcpy(ret, str, *retLen + 1);
    ret[*retLen + 1] = '\0';

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

static Value stringIndexOfFirst(VM *vm, int argc, Value *args) {
    if (argc != 1 && argc != 2) {
        runtimeError(vm, "Function indexOfFirst() expected 1 or 2 arguments but got %d.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function indexOfFirst() expected type 'string' for the first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    char *string = AS_CSTRING(args[0]);
    char *toFind = AS_CSTRING(args[1]);
    int startIndex = 0;

    if (argc == 2) {
        if (!IS_NUMBER(args[2])) {
            char *str = valueType(args[2]);
            runtimeError(vm, "Function indexOfFirst() expected type 'number' for the second argument but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }

        startIndex = AS_NUMBER(args[2]);
    }

    if (startIndex >= AS_STRING(args[0])->len || startIndex < 0) {
        return -1;
    }

    char *res = strstr(string + startIndex, toFind);
    if (res == nullptr) {
        return NUMBER_VAL(-1);
    }

    int pos = (int)(res - string);
    return NUMBER_VAL(pos);
}

static Value stringSplit(VM *vm, int argc, Value *args) {
    if (argc != 1 && argc != 2) {
        runtimeError(vm, "Function split() expected 1 or 2 arguments but got %d.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function split() expected type 'string' for the first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    ObjString *string = AS_STRING(args[0]);
    char *delim = AS_CSTRING(args[1]);
    int maxSplit = string->len + 1;

    if (argc == 2) {
        if (!IS_NUMBER(args[2])) {
            char *str = valueType(args[2]);
            runtimeError(vm, "Function split() expected type 'number' for the second argument but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }

        maxSplit = AS_NUMBER(args[2]);
    }

    char *tmp = ALLOCATE(vm, char, string->len + 1);
    char *tmpFree = tmp;
    memcpy(tmp, string->str, string->len);
    tmp[string->len] = '\0';
    int delimLen = (int)strlen(delim);
    char *token;
    ObjArray *arr = newArray(vm);
    push(vm, OBJ_VAL(arr));
    int count = 0;

    if (delimLen == 0) {
        // Split every char out.
        int tokenIdx = 0;
        for (; tokenIdx < string->len && count < maxSplit; ++tokenIdx) {
            ++count;
            *(tmp) = string->str[tokenIdx];
            *(tmp + 1) = '\0';
            Value str = OBJ_VAL(copyString(vm, tmp, 1));
            // Push to the stack to avoid the GC.
            push(vm, str);
            writeValueArray(vm, &arr->data, str);
            pop(vm);
        }

        if (tokenIdx != string->len && count >= maxSplit) {
            tmp = (string->str) + tokenIdx;
        } else {
            tmp = nullptr;
        }
    } else if (maxSplit > 0) {
        do {
            ++count;
            token = strstr(tmp, delim);
            if (token) {
                *token = '\0';
            }

            Value str = OBJ_VAL(copyString(vm, tmp, strlen(tmp)));
            // Push to the stack to avoid the GC.
            push(vm, str);
            writeValueArray(vm, &arr->data, str);
            pop(vm);

            tmp = token + delimLen;
        } while (token != nullptr && count < maxSplit);

        if (token == nullptr) {
            tmp = nullptr;
        }
    }

    if (tmp != nullptr && count >= maxSplit) {
        Value remainingStr = OBJ_VAL(copyString(vm, tmp, strlen(tmp)));
        // Push to the stack to avoid the GC.
        push(vm, remainingStr);
        writeValueArray(vm, &arr->data, remainingStr);
        pop(vm);
    }

    pop(vm); // arr
    FREE_ARRAY(vm, char, tmpFree, string->len + 1);
    return OBJ_VAL(arr);
}

static Value stringTrimStart(VM *vm, int argc, Value *args) {
    if (argc != 0) {
        runtimeError(vm, "Function trimStart() expected 0 arguments but got %d.", argc);
        return ERROR_VAL;
    }

    ObjString *string = AS_STRING(args[0]);
    int count = 0;
    char *temp = ALLOCATE(vm, char, string->len + 1);

    for (int i = 0; i < string->len; ++i) {
        if (!isspace(string->str[i])) {
            break;
        }
        count++;
    }

    if (count != 0) {
        temp = SHRINK_ARRAY(vm, temp, char, string->len + 1, (string->len - count) + 1);
    }

    memcpy(temp, string->str + count, string->len - count);
    temp[string->len - count] = '\0';
    return OBJ_VAL(takeString(vm, temp, string->len - count));
}

static Value stringTrimEnd(VM *vm, int argc, Value *args) {
    if (argc != 0) {
        runtimeError(vm, "Function trimEnd() expected 0 arguments but got %d.", argc);
        return ERROR_VAL;
    }

    ObjString *string = AS_STRING(args[0]);
    int len;
    char *temp = ALLOCATE(vm, char, string->len + 1);

    for (len = string->len - 1; len > 0; --len) {
        if (!isspace(string->str[len])) {
            break;
        }
    }

    if (len + 1 != string->len) {
        temp = SHRINK_ARRAY(vm, temp, char, string->len + 1, len + 2);
    }

    memcpy(temp, string->str, len + 1);
    temp[len + 1] = '\0';
    return OBJ_VAL(takeString(vm, temp, len + 1));
}

static Value stringTrim(VM *vm, const int argc, Value *args) {
    if (argc != 0) {
        runtimeError(vm, "Function trim() expected 0 arguments but got %d.", argc);
        return ERROR_VAL;
    }

    Value str = stringTrimStart(vm, 0, args);
    push(vm, str);
    str = stringTrimEnd(vm, 0, &str);
    pop(vm);
    return str;
}

static Value stringToString(VM *vm, int argc, Value *args) {
    return args[0];
}

void defineStringFunctions(VM *vm) {
    defineNative(vm, "toUpper", stringToUpper, &vm->stringFunctions);
    defineNative(vm, "toLower", stringToLower, &vm->stringFunctions);
    defineNative(vm, "len", stringLen, &vm->stringFunctions);
    defineNative(vm, "contains", stringContains, &vm->stringFunctions);
    defineNative(vm, "toNumber", stringToNumber, &vm->stringFunctions);
    defineNative(vm, "indexOfFirst", stringIndexOfFirst, &vm->stringFunctions);
    defineNative(vm, "split", stringSplit, &vm->stringFunctions);
    defineNative(vm, "trimStart", stringTrimStart, &vm->stringFunctions);
    defineNative(vm, "trimEnd", stringTrimEnd, &vm->stringFunctions);
    defineNative(vm, "trim", stringTrim, &vm->stringFunctions);
    defineNative(vm, "toString", stringToString, &vm->stringFunctions);
}