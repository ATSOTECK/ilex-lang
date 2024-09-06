//
// Created by Skyler on 11/23/23.
//

#include "lib_test.h"

#include "../vm.h"

static Value testAssertEq(VM *vm, int argc, Value *args) {
    return FALSE_VAL;
}

Value useTestLib(VM *vm) {
    ObjString *name = copyString(vm, "test", 4);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    if (lib->used) {
        return OBJ_VAL(lib);
    }

    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}