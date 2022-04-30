//
// Created by Skyler on 4/29/22.
//

#include "lib_math.h"

#include "../vm.h"

#include <math.h>
#include <stdlib.h>

static Value mathSqrt(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function sqrt() expected 1 argument but got %d.", argc);
        return NULL_VAL;
    }

    if (!IS_NUMBER(args[0])) {
        char *str = valueType(args[0]);
        runtimeError(vm, "Function sqrt() expected type 'number' but got '%s'.", str);
        free(str);

        return NULL_VAL;
    }

    return AS_NUMBER(sqrt(AS_NUMBER(args[0])));
}

Value useMathLib(VM *vm) {
    ObjString *name = copyString(vm, "math", 4);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    defineNative(vm, "math", mathSqrt, &vm->scripts);

    pop(vm);
    pop(vm);

    return OBJ_VAL(lib);
}
