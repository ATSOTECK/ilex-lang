//
// Created by Skyler on 11/23/23.
//

#include "lib_test.h"

#include "../vm.h"

static Value testTest(VM *vm, int argc, const Value *args) {
    return FALSE_VAL;
}

static Value testGroup(VM *vm, int argc, const Value *args) {
    return FALSE_VAL;
}

static Value testExpect(VM *vm, int argc, const Value *args) {
    return FALSE_VAL;
}

static Value testRequire(VM *vm, int argc, const Value *args) {
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

    defineNative(vm, "test", testTest, &lib->values);
    defineNative(vm, "group", testGroup, &lib->values);
    defineNative(vm, "expect", testExpect, &lib->values);
    defineNative(vm, "require", testRequire, &lib->values);

    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}