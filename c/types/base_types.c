//
// Created by Skyler Burwell on 2/19/25.
//

#include "base_types.h"

#include <stdlib.h>

static Value numberToStringLib(VM *vm, const int argc, const Value *args) {
    char *str = valueToString(args[0]);
    ObjString *ret = copyString(vm, str, (int)strlen(str));
    free(str);

    return OBJ_VAL(ret);
}

void defineNumberFunctions(VM *vm) {
    defineNative(vm, "toString", numberToStringLib, &vm->numberFunctions);
}
