//
// Created by Skyler on 7/26/22.
//

#include "lib_sys.h"

#include "../vm.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/syslimits.h>

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
    }

    exit((int)AS_NUMBER(args[0]));
    return ZERO_VAL;
}

static Value sysGetCWD(VM *vm, int argc, Value *args) {
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return OBJ_VAL(copyString(vm, cwd, strlen(cwd)));
    }

    return NULL_VAL;
}

static Value sysPWD(VM *vm, int argc, Value *args) {
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
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
    }

    char *dir = AS_CSTRING(args[0]);
    int ret = chdir(dir);
    return ret < 0 ? NULL_VAL : ZERO_VAL;
}

Value useSysLib(VM *vm) {
    ObjString *name = copyString(vm, "io", 2);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    defineNative(vm, "sleep", sysSleep, &lib->values);
    defineNative(vm, "exit", sysExit, &lib->values);
    defineNative(vm, "cwd", sysGetCWD, &lib->values);
    defineNative(vm, "pwd", sysPWD, &lib->values);
    defineNative(vm, "cd", sysCD, &lib->values);

    pop(vm);
    pop(vm);

    return OBJ_VAL(lib);
}
