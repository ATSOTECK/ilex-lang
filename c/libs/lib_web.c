//
// Created by Skyler Burwell on 2/5/25.
//

#include "lib_web.h"

#include "../vm.h"

static Value webNewApp(VM *vm, const int argc, Value *args) {
    return FALSE_VAL;
}

Value useWebLib(VM *vm) {
    ObjString *name = copyString(vm, "web", 3);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    if (lib->used) {
        return OBJ_VAL(lib);
    }

    defineNative(vm, "app", webNewApp, &lib->values);

    pop(vm);
    pop(vm);

    lib->used = true;
    return OBJ_VAL(lib);
}
