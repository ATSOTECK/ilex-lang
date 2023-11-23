//
// Created by Skyler on 4/29/22.
//

#include "lib_builtIn.h"

#include "../memory.h"

BuiltInLibs makeLib(VM *vm, const char *name, BuiltInLib lib) {
    BuiltInLibs ret;
    int len = (int)strlen(name);
    ret.name = ALLOCATE(vm, char, len + 1);
    memcpy(ret.name, name, len);
    ret.name[len] = '\0';
    ret.lib = lib;

    return ret;
}

void initBuiltInLibs(VM *vm) {
    vm->libCapacity = 0;
    vm->libCapacity = GROW_CAPACITY(vm->libCapacity);

    vm->libCount = 7;
    vm->libs = ALLOCATE(vm, BuiltInLibs, vm->libCapacity);
    vm->libs[0] = makeLib(vm, "math",   &useMathLib);
    vm->libs[1] = makeLib(vm, "ilex",   &useIlexLib);
    vm->libs[2] = makeLib(vm, "io",     &useIoLib);
    vm->libs[3] = makeLib(vm, "random", &useRandomLib);
    vm->libs[4] = makeLib(vm, "env",    &useEnvLib);
    vm->libs[5] = makeLib(vm, "sys",    &useSysLib);
    vm->libs[6] = makeLib(vm, "json",   &useJsonLib);
    vm->libs[7] = makeLib(vm, "window",   &useWindowLib);
}

Value useBuiltInLib(VM *vm, int idx) {
    return vm->libs[idx].lib(vm);
}

int findBuiltInLib(VM *vm, char *name, int len) {
    for (int i = 0; vm->libs[i].lib != NULL; ++i) {
        if (strncmp(vm->libs[i].name, name, len) == 0) {
            return i;
        }
    }

    return -1;
}