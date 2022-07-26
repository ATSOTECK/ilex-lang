//
// Created by Skyler on 7/24/22.
//

#include "lib_map.h"

#include "../util.h"

#include <stdlib.h>

static Value mapSize(VM *vm, int argc, Value *args) {
    ObjMap *map = AS_MAP(args[0]);
    return NUMBER_VAL(map->count);
}

static Value mapMaxSize(VM *vm, int argc, Value *args) {
    ObjMap *map = AS_MAP(args[0]);
    return NUMBER_VAL(map->capacity + 1);
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

static Value mapValues(VM *vm, int argc, Value *args) {
    ObjMap *map = AS_MAP(args[0]);
    ObjArray *keys = newArray(vm);
    push(vm, OBJ_VAL(keys));
    
    for (int i = 0; i < map->capacity + 1; ++i) {
        if (IS_ERR(map->items[i].key)) {
            continue;
        }
        
        writeValueArray(vm, &keys->data, map->items[i].value);
    }
    
    pop(vm);
    return OBJ_VAL(keys);
}

static Value mapGetLib(VM *vm, int argc, Value *args) {
    if (argc == 0 || argc > 2) {
        runtimeError(vm, "Function get() expected 1 or 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    Value defaultValue = NULL_VAL;
    if (argc == 2) {
        defaultValue = args[2];
    }
    
    if (!isValidKey(args[1])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Expect string or number for key but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }
    
    ObjMap *map = AS_MAP(args[0]);
    Value ret;
    
    if (mapGet(map, args[1], &ret)) {
        return ret;
    }
    
    return defaultValue;
}

static Value mapDeleteLib(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function delete() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!isValidKey(args[1])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Expect string or number for key but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }
    
    ObjMap *map = AS_MAP(args[0]);
    
    if (mapDelete(vm, map, args[1])) {
        return TRUE_VAL;
    }
    
    return FALSE_VAL;
}

static Value mapExists(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function exists() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!isValidKey(args[1])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Expect string or number for key but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }
    
    ObjMap *map = AS_MAP(args[0]);
    Value _;
    
    if (mapGet(map, args[1], &_)) {
        return TRUE_VAL;
    }
    
    return FALSE_VAL;
}

static Value mapIsEmpty(VM *vm, int argc, Value *args) {
    ObjMap *map = AS_MAP(args[0]);
    return map->count == 0 ? TRUE_VAL : FALSE_VAL;
}

void defineMapFunctions(VM *vm) {
    defineNative(vm, "size", mapSize, &vm->mapFunctions);
    defineNative(vm, "maxSize", mapMaxSize, &vm->mapFunctions);
    defineNative(vm, "toString", mapToStringLib, &vm->mapFunctions);
    defineNative(vm, "keys", mapKeys, &vm->mapFunctions);
    defineNative(vm, "values", mapValues, &vm->mapFunctions);
    defineNative(vm, "get", mapGetLib, &vm->mapFunctions);
    defineNative(vm, "delete", mapDeleteLib, &vm->mapFunctions);
    defineNative(vm, "exists", mapExists, &vm->mapFunctions);
    defineNative(vm, "isEmpty", mapIsEmpty, &vm->mapFunctions);
}
