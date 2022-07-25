//
// Created by Skyler on 7/24/22.
//

#include "lib_map.h"

static Value mapLen(VM *vm, int argc, Value *args) {
    ObjMap *map = AS_MAP(args[0]);
    return NUMBER_VAL(map->count);
}

static Value mapToStringLib(VM *vm, int argc, Value *args) {
    char *str = mapToString(AS_MAP(args[0]));
    ObjString *ret = copyString(vm, str, (int)strlen(str));
    free(str);
    
    return OBJ_VAL(ret);
}

static Value mapKeys(VM *vm, int argc, Value *args) {
    ObjMap *map = AS_MAP(args[0]);
    ObjArray *keys = newArray(vm);
    push(vm, OBJ_VAL(keys));
    
    for (int i = 0; i < map->capacity + 1; ++i) {
        if (IS_ERR(map->items[i].key)) {
            continue;
        }
    
        writeValueArray(vm, &keys->data, map->items[i].key);
    }
    
    pop(vm);
    return OBJ_VAL(keys);
}

void defineMapFunctions(VM *vm) {
    defineNative(vm, "len", mapLen, &vm->mapFunctions);
    defineNative(vm, "toString", mapToStringLib, &vm->mapFunctions);
    defineNative(vm, "keys", mapKeys, &vm->mapFunctions);
}
