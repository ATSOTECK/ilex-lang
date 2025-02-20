//
// Created by Skyler on 7/24/22.
//

#include "type_map.h"

#include "../util.h"

#include <stdlib.h>

static Value mapSizeLib(VM *vm, int argc, Value *args) {
    const ObjMap *map = AS_MAP(args[0]);
    return NUMBER_VAL(map->count);
}

static Value mapCapacityLib(VM *vm, int argc, Value *args) {
    const ObjMap *map = AS_MAP(args[0]);
    return NUMBER_VAL(map->capacity + 1);
}

static Value mapToStringLib(VM *vm, int argc, Value *args) {
    char *str = mapToString(AS_MAP(args[0]));
    ObjString *ret = copyString(vm, str, (int)strlen(str));
    free(str);
    
    return OBJ_VAL(ret);
}

static Value mapKeysLib(VM *vm, int argc, Value *args) {
    const ObjMap *map = AS_MAP(args[0]);
    const ValueArray keysArr = mapKeys(vm, map);
    ObjArray *keys = newArray(vm);
    keys->data = keysArr;

    return OBJ_VAL(keys);
}

static Value mapValuesLib(VM *vm, int argc, Value *args) {
    const ObjMap *map = AS_MAP(args[0]);
    const ValueArray valuesArr = mapValues(vm, map);
    ObjArray *values = newArray(vm);
    values->data = valuesArr;

    return OBJ_VAL(values);
}

static Value mapGetLib(VM *vm, const int argc, Value *args) {
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
    
    const ObjMap *map = AS_MAP(args[0]);
    Value ret;
    
    if (mapGet(map, args[1], &ret)) {
        return ret;
    }
    
    return defaultValue;
}

static Value mapPopLib(VM *vm, const int argc, Value *args) {
    if (argc == 0 || argc > 2) {
        runtimeError(vm, "Function pop() expected 1 or 2 arguments but got '%d'.", argc);
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
        mapDelete(vm, map, args[1]);
        return ret;
    }

    return defaultValue;
}

static Value mapDeleteLib(VM *vm, const int argc, Value *args) {
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

static Value mapClearLib(VM *vm, int argc, Value *args) {
    ObjMap *map = AS_MAP(args[0]);
    mapClear(vm, map);

    return TRUE_VAL;
}

static Value mapExistsLib(VM *vm, const int argc, Value *args) {
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
    
    const ObjMap *map = AS_MAP(args[0]);
    if (mapHasKey(map, args[1])) {
        return TRUE_VAL;
    }
    
    return FALSE_VAL;
}

static Value mapIsEmpty(VM *vm, int argc, Value *args) {
    const ObjMap *map = AS_MAP(args[0]);
    return map->count == 0 ? TRUE_VAL : FALSE_VAL;
}

static Value mapCopyShallow(VM *vm, int argc, Value *args) {
    ObjMap *map = AS_MAP(args[0]);
    ObjMap *ret = copyMap(vm, map, true);
    return OBJ_VAL(ret);
}

static Value mapCopyDeep(VM *vm, int argc, Value *args) {
    ObjMap *map = AS_MAP(args[0]);
    ObjMap *ret = copyMap(vm, map, false);
    return OBJ_VAL(ret);
}

static Value mapForEach(VM *vm, const int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function forEach() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_CLOSURE(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function forEach() expected type 'closure' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    const ObjMap *map = AS_MAP(args[0]);
    ObjClosure *closure = AS_CLOSURE(args[1]);
    Value *fnArgs = (Value*)malloc(sizeof(Value) * 2);

    for (int i = 0; i < map->capacity + 1; ++i) {
        if (IS_ERR(map->items[i].key)) {
            continue;
        }

        fnArgs[0] = map->items[i].key;
        fnArgs[1] = map->items[i].value;

        const Value ret = callFromScript(vm, closure, 2, fnArgs);

        if (IS_ERR(ret)) {
            return ERROR_VAL;
        }
    }

    free(fnArgs);

    return ZERO_VAL;
}

static Value mapMergeLib(VM *vm, const int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function merge() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_MAP(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function merge() expected type 'map' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    ObjMap *map = AS_MAP(args[0]);
    const ObjMap *other = AS_MAP(args[1]);

    for (int i = 0; i < other->capacity + 1; ++i) {
        const MapItem *item = &other->items[i];
        if (IS_ERR(item->key)) {
            continue;
        }

        if (!mapHasKey(map, item->key)) {
            mapSet(vm, map, item->key, item->value);
        }
    }

    return TRUE_VAL;
}

static Value mapUpdateLib(VM *vm, const int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function update() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }

    if (!IS_MAP(args[1])) {
        char *str = valueType(args[1]);
        runtimeError(vm, "Function update() expected type 'map' for first argument but got '%s'.", str);
        free(str);
        return ERROR_VAL;
    }

    ObjMap *map = AS_MAP(args[0]);
    const ObjMap *other = AS_MAP(args[1]);

    for (int i = 0; i < other->capacity + 1; ++i) {
        const MapItem *item = &other->items[i];
        if (IS_ERR(item->key)) {
            continue;
        }

        mapSet(vm, map, item->key, item->value);
    }

    return TRUE_VAL;
}

void defineMapFunctions(VM *vm) {
    defineNative(vm, "size", mapSizeLib, &vm->mapFunctions);
    defineNative(vm, "capacity", mapCapacityLib, &vm->mapFunctions);
    defineNative(vm, "toString", mapToStringLib, &vm->mapFunctions);
    defineNative(vm, "keys", mapKeysLib, &vm->mapFunctions);
    defineNative(vm, "values", mapValuesLib, &vm->mapFunctions);
    defineNative(vm, "get", mapGetLib, &vm->mapFunctions);
    defineNative(vm, "pop", mapPopLib, &vm->mapFunctions);
    defineNative(vm, "delete", mapDeleteLib, &vm->mapFunctions);
    defineNative(vm, "clear", mapClearLib, &vm->mapFunctions);
    defineNative(vm, "exists", mapExistsLib, &vm->mapFunctions);
    defineNative(vm, "isEmpty", mapIsEmpty, &vm->mapFunctions);
    defineNative(vm, "shallowCopy", mapCopyShallow, &vm->mapFunctions);
    defineNative(vm, "deepCopy", mapCopyDeep, &vm->mapFunctions);
    defineNative(vm, "merge", mapMergeLib, &vm->mapFunctions);
    defineNative(vm, "update", mapUpdateLib, &vm->mapFunctions);

    defineNative(vm, "forEach", mapForEach, &vm->mapFunctions);
}
