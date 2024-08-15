//
// Created by Skyler on 8/14/24.
//

#include "type_enum.h"

static Value enumValues(VM *vm, int argc, Value *args) {
    if (argc != 0) {
        runtimeError(vm, "Function values() takes no arguments but got '%d'.", argc);
        return ERROR_VAL;
    }

    ObjEnum *objEnum = AS_ENUM(args[0]);
    ObjMap *map = newMap(vm);
    push(vm, OBJ_VAL(map));

    for (int i = 0; i < objEnum->values.capacity; ++i) {
        if (objEnum->values.entries[i].key == NULL) {
            continue;
        }

        mapSet(vm, map, OBJ_VAL(objEnum->values.entries[i].key), objEnum->values.entries[i].value);
    }

    pop(vm);

    return OBJ_VAL(map);
}

void defineEnumFunctions(VM *vm) {
    defineNative(vm, "values", enumValues, &vm->enumFunctions);
}
