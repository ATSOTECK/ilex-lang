//
// Created by Skyler on 7/28/22.
//

#include "lib_set.h"

#include "../util.h"

#include <stdlib.h>

static Value setToStringLib(VM *vm, int argc, Value *args) {
    char *str = setToString(AS_SET(args[0]));
    ObjString *ret = copyString(vm, str, (int)strlen(str));
    free(str);
    
    return OBJ_VAL(ret);
}

static Value setSize(VM *vm, int argc, Value *args) {
    ObjSet *set = AS_SET(args[0]);
    return NUMBER_VAL(set->count);
}

static Value setMaxSize(VM *vm, int argc, Value *args) {
    ObjSet *set = AS_SET(args[0]);
    return NUMBER_VAL(set->capacity + 1);
}

static Value setAddLib(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function add() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!isValidKey(args[1])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Expect string or number for value but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }
    
    ObjSet *set = AS_SET(args[0]);
    return setAdd(vm, set, args[1]) ? TRUE_VAL : FALSE_VAL;
}

static Value setContains(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function add() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!isValidKey(args[1])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Expect string or number for value but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }
    
    ObjSet *set = AS_SET(args[0]);
    return setGet(vm, set, args[1]) ? TRUE_VAL : FALSE_VAL;
}


static Value setDeleteLib(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function add() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!isValidKey(args[1])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Expect string or number for value but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }
    
    ObjSet *set = AS_SET(args[0]);
    return setDelete(vm, set, args[1]) ? TRUE_VAL : FALSE_VAL;
}

static Value setIsEmpty(VM *vm, int argc, Value *args) {
    ObjSet *set = AS_SET(args[0]);
    return set->count == 0 ? TRUE_VAL : FALSE_VAL;
}

void defineSetFunctions(VM *vm) {
    defineNative(vm, "toString", setToStringLib, &vm->setFunctions);
    defineNative(vm, "size", setSize, &vm->setFunctions);
    defineNative(vm, "maxSize", setMaxSize, &vm->setFunctions);
    defineNative(vm, "add", setAddLib, &vm->setFunctions);
    defineNative(vm, "contains", setContains, &vm->setFunctions);
    defineNative(vm, "delete", setDeleteLib, &vm->setFunctions);
    defineNative(vm, "isEmpty", setIsEmpty, &vm->setFunctions);
}
