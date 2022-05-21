//
// Created by Skyler on 05/20/22.
//

#include "lib_file.h"

static Value fileWrite(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function write() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!IS_STRING(args[1])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Function write() expected type 'string' for first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }
    
    ObjFile *file = AS_FILE(args[0]);
    ObjString *str = AS_STRING(args[1]);
    
    if (strcmp(file->flags, "r") == 0) {
        runtimeError(vm, "File is not writeable.");
        return ERROR_VAL;
    }
    
    int written = fprintf(file->file, "%s", str->str);
    fflush(file->file);
    
    return NUMBER_VAL(written);
}

static Value fileWriteLine(VM *vm, int argc, Value *args) {
    if (argc != 1) {
        runtimeError(vm, "Function write() expected 1 argument but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!IS_STRING(args[1])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Function write() expected type 'string' for first argument but got '%s'.", type);
        free(type);
        return ERROR_VAL;
    }
    
    ObjFile *file = AS_FILE(args[0]);
    ObjString *str = AS_STRING(args[1]);
    
    if (strcmp(file->flags, "r") == 0) {
        runtimeError(vm, "File is not writeable.");
        return ERROR_VAL;
    }
    
    int written = fprintf(file->file, "%s\n", str->str);
    fflush(file->file);
    
    return NUMBER_VAL(written);
}

void defineFileFunctions(VM *vm) {
    defineNative(vm, "write", fileWrite, &vm->fileFunctions);
    defineNative(vm, "writeln", fileWriteLine, &vm->fileFunctions);
}
