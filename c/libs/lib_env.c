//
// Created by Skyler on 5/19/22.
//

#include "lib_env.h"

#include "../vm.h"

#include <stdlib.h>
#include <stdio.h>

#define RM_BRACKET(name) name + 1
#define RM_SPACE(value) value + 1

static bool isCommented(const char *line) {
    if (line[0] == '#') {
        return true;
    }

    int i = 0;
    while (line[i] == ' ' || line[i] == '\t') {
        if (line[++i] == '#') {
            return true;
        }
    }

    return false;
}

static char *concat(char *buffer, char *str) {
    if (!buffer) {
        return strdup(str);
    }

    if (str) {
        size_t len = strlen(buffer) + strlen(str) + 1;
        char *new = (char*)realloc(buffer, len);

        return strcat(new, str);
    }

    return buffer;
}

static char *parseValue(char *value) {
    char *new = (char*)malloc(sizeof(char) * strlen(value) + 2);
    sprintf(new, " %s", value);
    value = new;

    char *search = value;
    char *parsed = NULL;
    char *ptr, *name;

    if (value && strstr(value, "${") && strstr(value, "}")) {
        for (;;) {
            parsed = concat(parsed, strtok_r(search, "${", &ptr));
            name = strtok_r(NULL, "}", &ptr);

            if (!name) {
                break;
            }

            parsed = concat(parsed, getenv(RM_BRACKET(name)));
            search = NULL;
        }

        free(value);

        return parsed;
    }

    return value;
}

static bool readEnv(VM *vm, const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        return false;
    }

    char *key, *value, *ptr;
    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, file) != -1) {
        if (!isCommented(line)) {
            key = strtok_r(line, "=", &ptr);
            value = strtok_r(NULL, "\n", &ptr);

            char *parsed;

            if (value) {
                parsed = parseValue(value);
                setenv(key, RM_SPACE(parsed), 1);

                free(parsed);
            }
        }
    }

    free(line);
    fclose(file);

    vm->envLoaded = true;
    return true;
}

static Value envGet(VM *vm, int argc, Value *args) {
    if (argc == 0 || argc > 2) {
        runtimeError(vm, "Function indexOf() expected 1 or 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *str = valueType(args[0]);
        runtimeError(vm, "Function get() expected type 'string' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    if (!vm->envLoaded && !readEnv(vm, ".env")) {
        runtimeError(vm, "Could not open '.env' for reading.");
        return ERROR_VAL;
    }

    char *envStr = getenv(AS_CSTRING(args[0]));
    if (envStr != NULL) {
        return OBJ_VAL(copyString(vm, envStr, strlen(envStr)));
    }

    if (argc == 2) {
        if (!IS_STRING(args[1])) {
            char *str = valueType(args[1]);
            runtimeError(vm, "Function get() expected type 'string' for first argument but got '%s'.", str);
            free(str);
            return ERROR_VAL;
        }

        return args[1];
    }

    return NULL_VAL;
}

static Value envSet(VM *vm, int argc, Value *args) {
    if (argc != 2) {
        runtimeError(vm, "Function set() expected 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function set() expected type 'string' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[1]) && !IS_NULL(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function set() expected type 'string' or 'null' for second argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    char *envKey = AS_CSTRING(args[0]);

    int retval;
    if (IS_NULL(args[1])) {
        retval = unsetenv(envKey);
    } else {
        retval = setenv(envKey, AS_CSTRING(args[1]), 1);
    }

    if (retval == -1) {
        return NULL_VAL;
    }

    return ZERO_VAL;
}

static Value envReadFile(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function readFile() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function set() expected type 'string' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    char *path = AS_CSTRING(args[0]);

    if (!readEnv(vm, path)) {
        runtimeError(vm, "Could not open '%s' for reading.", path);
        return ERROR_VAL;
    }

    return ZERO_VAL;
}

Value useEnvLib(VM *vm) {
    ObjString *name = copyString(vm, "env", 4);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    defineNative(vm, "get", envGet, &lib->values);
    defineNative(vm, "set", envSet, &lib->values);
    defineNative(vm, "load", envReadFile, &lib->values);

    pop(vm);
    pop(vm);

    return OBJ_VAL(lib);
}
