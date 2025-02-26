//
// Created by Skyler Burwell on 2/25/25.
//

#include "lib_base64.h"

#include <stdlib.h>

const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const int base64DecodeTable[] = {
    62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58,
    59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5,
    6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51
};

static int base64EncodedSize(const int len) {
    int ret = len;
    if (len % 3 != 0) {
        ret += 3 - len % 3;
    }
    ret /= 3;

    return ret * 4;
}

static ObjString *base64Encode(VM* vm, const ObjString *str) {
    if (str == NULL || str->len == 0) {
        return copyString(vm, "", 0);
    }

    const int encodedLen = base64EncodedSize(str->len);
    char *encodedStr = (char*)malloc(sizeof(char) * encodedLen + 1);
    encodedStr[encodedLen] = '\0';

    for (int i = 0, j = 0; i < str->len; i += 3, j += 4) {
        int v = (int)str->str[i];
        v = i + 1 < str->len ? v << 8 | str->str[i + 1] : v << 8;
        v = i + 2 < str->len ? v << 8 | str->str[i + 2] : v << 8;

        encodedStr[j]     = base64Chars[v >> 18 & 0x3F];
        encodedStr[j + 1] = base64Chars[v >> 12 & 0x3F];

        if (i + 1 < str->len) {
            encodedStr[j + 2] = base64Chars[v >> 6 & 0x3F];
        } else {
            encodedStr[j + 2] = '=';
        }

        if (i + 2 < str->len) {
            encodedStr[j + 3] = base64Chars[v & 0x3F];
        } else {
            encodedStr[j + 3] = '=';
        }
    }

    return takeString(vm, encodedStr, encodedLen);
}

static int base64DecodedSize(const ObjString *str) {
    if (str == NULL || str->len == 0) {
        return 0;
    }

    int ret = str->len / 4 * 3;
    for (int i = str->len; i --> 0; ) {
        if (str->str[i] == '=') {
            --ret;
        } else {
            break;
        }
    }

    return ret;
}

static bool base64IsValidChar(const char c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '+' || c == '/' || c == '=';
}

static ObjString *base64Decode(VM *vm, const ObjString *str) {
    if (str == NULL || str->len == 0) {
        return copyString(vm, "", 0);
    }

    for (int i = 0; i < str->len; ++i) {
        if (!base64IsValidChar(str->str[i])) {
            return copyString(vm, "", 0);
        }
    }

    const int decodedLen = base64DecodedSize(str);
    char *decodedStr = (char*)malloc(sizeof(char) * decodedLen + 1);

    for (int i = 0, j = 0; i < str->len; i += 4, j += 3) {
        int v = base64DecodeTable[str->str[i] - 43];
        v = v << 6 | base64DecodeTable[str->str[i + 1] - 43];
        v = str->str[i + 2] == '=' ? v << 6 : v << 6 | base64DecodeTable[str->str[i + 2] - 43];
        v = str->str[i + 3] == '=' ? v << 6 : v << 6 | base64DecodeTable[str->str[i + 3] - 43];

        decodedStr[j] = (char)(v >> 16 & 0xFF);
        if (str->str[i + 2] != '=') {
            decodedStr[j + 1] = (char)(v >> 8 & 0xFF);
        }
        if (str->str[i + 3] != '=') {
            decodedStr[j + 2] = (char)(v & 0xFF);
        }
    }

    return takeString(vm, decodedStr, decodedLen);
}

static Value base64DecodeLib(VM *vm, const int argc, const Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function decode() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function decode() expected type 'string' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    const ObjString *str = AS_STRING(args[0]);
    return OBJ_VAL(base64Decode(vm, str));
}

static Value base64EncodeLib(VM *vm, const int argc, const Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function encode() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function encode() expected type 'string' for the first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    const ObjString *str = AS_STRING(args[0]);
    return OBJ_VAL(base64Encode(vm, str));
}

Value useBase64Lib(VM *vm) {
    ObjString *name = copyString(vm, "base64", 6);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    if (lib->used) {
        return OBJ_VAL(lib);
    }

    defineNative(vm, "decode", base64DecodeLib, &lib->values);
    defineNative(vm, "encode", base64EncodeLib, &lib->values);

    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}
