//
// Created by Skyler on 5/2/22.
//

#include "lib_io.h"

#include "../memory.h"
#include "../vm.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static Value ioInput(VM *vm, int argc, Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function input() expected 1 or 0 arguments but got %d", argc);
        return ERROR_VAL;
    }

    if (argc != 0) {
        Value prompt = args[0];
        if (!IS_STRING(prompt)) {
            char *str = valueType(args[1]);
            runtimeError(vm, "Function input() expected type 'string' but got '%s'.", str);
            free(str);

            return ERROR_VAL;
        }

        printf("%s", AS_CSTRING(prompt));
    }

    uint64_t currentSize = 128;
    char *line = ALLOCATE(vm, char, currentSize);

    if (line == NULL) {
        runtimeError(vm, "Memory error on input()!");
        return ERROR_VAL;
    }

    int c = EOF;
    uint64_t length = 0;
    while ((c = getchar()) != '\n' && c != EOF) {
        line[length++] = (char) c;

        if (length + 1 == currentSize) {
            uint64_t oldSize = currentSize;
            currentSize = GROW_CAPACITY(currentSize);
            line = GROW_ARRAY(vm, char, line, oldSize, currentSize);

            if (line == NULL) {
                printf("Unable to allocate memory\n");
                exit((int)INTERPRET_RUNTIME_ERROR);
            }
        }
    }

    // If length has changed then shrink.
    if (length != currentSize) {
        line = SHRINK_ARRAY(vm, line, char, currentSize, length + 1);
    }

    line[length] = '\0';

    return OBJ_VAL(takeString(vm, line, length));
}

static Value ioGetNumber(VM *vm, int argc, Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function getNumber() expected 1 or 0 arguments but got %d", argc);
        return ERROR_VAL;
    }
    
    if (argc != 0) {
        Value prompt = args[0];
        if (!IS_STRING(prompt)) {
            char *str = valueType(prompt);
            runtimeError(vm, "Function getNumber() expected type 'string' but got '%s'.", str);
            free(str);
            
            return ERROR_VAL;
        }
        
        printf("%s", AS_CSTRING(prompt));
    }
    
    uint64_t currentSize = 128;
    char *line = ALLOCATE(vm, char, currentSize);
    
    if (line == NULL) {
        runtimeError(vm, "Memory error on input()!");
        return ERROR_VAL;
    }
    
    int c = EOF;
    uint64_t length = 0;
    while ((c = getchar()) != '\n' && c != EOF) {
        line[length++] = (char) c;
        
        if (length + 1 == currentSize) {
            uint64_t oldSize = currentSize;
            currentSize = GROW_CAPACITY(currentSize);
            line = GROW_ARRAY(vm, char, line, oldSize, currentSize);
            
            if (line == NULL) {
                printf("Unable to allocate memory\n");
                exit((int)INTERPRET_RUNTIME_ERROR);
            }
        }
    }
    
    // If length has changed then shrink.
    if (length != currentSize) {
        line = SHRINK_ARRAY(vm, line, char, currentSize, length + 1);
    }
    
    line[length] = '\0';
    
    char *end;
    double number = strtod(line, &end);
    
    if (errno != 0 || *end != '\0') {
        return NULL_VAL;
    }
    
    return NUMBER_VAL(number);
}

static Value ioOpenFile(VM *vm, int argc, Value *args) {
    if (argc != 2) {
        runtimeError(vm, "Function openFile() expected 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!IS_STRING(args[0])) {
        char *type = valueType(args[0]);
        runtimeError(vm, "Function openFile() expected type 'string' for first argument but got '%s'.", type);
        free(type);
        
        return ERROR_VAL;
    }
    
    if (!IS_STRING(args[1])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Function openFile() expected type 'string' for second argument but got '%s'.", type);
        free(type);
        
        return ERROR_VAL;
    }
    
    ObjString *nameStr = AS_STRING(args[0]);
    ObjString *flagStr = AS_STRING(args[1]);
    
    ObjFile *file = newFile(vm);
#ifdef I_WIN
    errno_t err = fopen_s(&file->file, nameStr->str, flagStr->str);
#else
    file->file = fopen(nameStr->str, flagStr->str);
    errno_t err = file->file == NULL ? -1 : 0;
#endif
    file->path = nameStr->str;
    file->flags = flagStr->str;
    
    if (err != 0) {
        runtimeError(vm, "Unable to open file '%s'.", file->path);
        return ERROR_VAL;
    }
    
    return OBJ_VAL(file);
}

Value useIoLib(VM *vm) {
    ObjString *name = copyString(vm, "io", 2);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    defineNative(vm, "input", ioInput, &lib->values);
    defineNative(vm, "getNumber", ioGetNumber, &lib->values);
    
    defineNative(vm, "openFile", ioOpenFile, &lib->values);

    pop(vm);
    pop(vm);

    return OBJ_VAL(lib);
}
