//
// Created by Skyler on 4/29/22.
//

#include "lib_builtIn.h"

#include "../memory.h"

BuiltInLibs makeLib(VM *vm, const char *name, const BuiltInLib lib) {
    BuiltInLibs ret;
    int len = (int)strlen(name);
    ret.name = ALLOCATE(vm, char, len + 1);
    memcpy(ret.name, name, len);
    ret.name[len] = '\0';
    ret.lib = lib;

    return ret;
}

void initBuiltInLibs(VM *vm) {
    vm->libCapacity = 9;
    vm->libCapacity = GROW_CAPACITY(vm->libCapacity);

    vm->libCount = 12;
    vm->libs = ALLOCATE(vm, BuiltInLibs, vm->libCapacity);
    vm->libs[0]  = makeLib(vm, "math",   &useMathLib);
    vm->libs[1]  = makeLib(vm, "ilex",   &useIlexLib);
    vm->libs[2]  = makeLib(vm, "io",     &useIoLib);
    vm->libs[3]  = makeLib(vm, "random", &useRandomLib);
    vm->libs[4]  = makeLib(vm, "env",    &useEnvLib);
    vm->libs[5]  = makeLib(vm, "sys",    &useSysLib);
    vm->libs[6]  = makeLib(vm, "json",   &useJsonLib);
    vm->libs[7]  = makeLib(vm, "window", &useWindowLib);
    vm->libs[8]  = makeLib(vm, "ascii",  &useAsciiLib);
    vm->libs[9]  = makeLib(vm, "web",    &useWebLib);
    vm->libs[10] = makeLib(vm, "http",   &useHttpLib);
    vm->libs[11] = makeLib(vm, "toml",   &useTomlLib);
}

Value useBuiltInLib(VM *vm, int idx) {
    return vm->libs[idx].lib(vm);
}

int findBuiltInLib(const VM *vm, const char *name, int len) {
    for (int i = 0; vm->libs[i].lib != NULL; ++i) {
        if (strncmp(vm->libs[i].name, name, len) == 0) {
            return i;
        }
    }

    return -1;
}