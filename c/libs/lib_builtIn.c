//
// Created by Skyler on 4/29/22.
//

#include "lib_builtIn.h"

BuiltInLibs libs[] = {
        {"math",   &useMathLib},
        {"ilex",   &useIlexLib},
        {"io",     &useIoLib},
        {"random", &useRandomLib},
        {"env",    &useEnvLib},
        {"sys",    &useSysLib},
        {NULL, NULL},
};

Value useBuiltInLib(VM *vm, int idx) {
    return libs[idx].lib(vm);
}

int findBuiltInLib(char *name, int len) {
    for (int i = 0; libs[i].lib != NULL; ++i) {
        if (strncmp(libs[i].name, name, len) == 0) {
            return i;
        }
    }

    return -1;
}