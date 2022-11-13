//
// Created by Skyler on 7/26/22.
//

#include "lib_sys.h"

#include "../vm.h"

#include <stdlib.h>
#ifndef I_WIN
#   include <unistd.h>
#   include <sys/syslimits.h>
#   include <sys/stat.h>
#   define _getcwd getcwd
#else
#   include <direct.h>
#endif

static Value sysSleep(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function sleep() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_NUMBER(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function sleep() expected type 'number' but got '%s'.", type);
        free(type);
    }

    double ms = AS_NUMBER(args[0]);

#ifdef I_WIN
    Sleep(ms);
#else
    usleep((useconds_t)ms * 1000);
#endif

    return ZERO_VAL;
}

static Value sysExit(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function sleep() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_NUMBER(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function sleep() expected type 'number' but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    exit((int)AS_NUMBER(args[0]));
    return ZERO_VAL;
}

static Value sysGetCWD(VM *vm, int argc, Value *args) {
    char cwd[PATH_MAX];

    if (_getcwd(cwd, sizeof(cwd)) != NULL) {
        return OBJ_VAL(copyString(vm, cwd, strlen(cwd)));
    }

    return NULL_VAL;
}

static Value sysPWD(VM *vm, int argc, Value *args) {
    char cwd[PATH_MAX];

    if (_getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return ZERO_VAL;
    }

    return NULL_VAL;
}

static Value sysCD(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function cd() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function cd() expected type 'string' but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    char *dir = AS_CSTRING(args[0]);
    int ret = chdir(dir);
    return ret < 0 ? NULL_VAL : ZERO_VAL;
}

static Value sysRMDIR(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function rmdir() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function rmdir() expected type 'string' but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    char *dir = AS_CSTRING(args[0]);
    int ret = rmdir(dir);

    return NUMBER_VAL(ret);
}

static Value sysMKDIR(VM *vm, int argc, Value *args) {
    if (argc != 1 && argc != 2) {
        runtimeError(vm, "Function mkdir() expected 1 or 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function mkdir() expected type 'string' for first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }

    char *dir = AS_CSTRING(args[0]);
    int mode = 0777;

    if (argc == 2) {
        if (!IS_NUMBER(args[1])) {
            char *type = valueType(args[1]);
            runtimeError(vm, "Function mkdir() expected type 'number' for second argument but got '%s'.", type);
            free(type);
            return ERROR_VAL;
        }

        mode = AS_NUMBER(args[1]);
    }

    int ret = mkdir(dir, mode);

    return NUMBER_VAL(ret);
}

Value useSysLib(VM *vm) {
    ObjString *name = copyString(vm, "io", 2);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));
    
    if (lib->used) {
        return OBJ_VAL(lib);
    }

    defineNative(vm, "sleep", sysSleep, &lib->values);
    defineNative(vm, "exit", sysExit, &lib->values);
    defineNative(vm, "cwd", sysGetCWD, &lib->values);
    defineNative(vm, "pwd", sysPWD, &lib->values);
    defineNative(vm, "cd", sysCD, &lib->values);
    defineNative(vm, "rmdir", sysRMDIR, &lib->values);
    defineNative(vm, "mkdir", sysMKDIR, &lib->values);

    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}
