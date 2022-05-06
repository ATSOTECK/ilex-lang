//
// Created by Skyler on 5/2/22.
//

#include "lib_io.h"

#include "../memory.h"
#include "../vm.h"

#include <stdio.h>
#include <stdlib.h>

static Value ioInput(VM *vm, int argc, Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function input() expected 1 or 0 arguments but got %d", argc);
        return NULL_VAL;
    }

    if (argc != 0) {
        Value prompt = args[0];
        if (!IS_STRING(prompt)) {
            char *str = valueType(args[1]);
            runtimeError(vm, "Function input() expected type 'string' but got '%s'.", str);
            free(str);

            return NULL_VAL;
        }

        printf("%s", AS_CSTRING(prompt));
    }

    uint64_t currentSize = 128;
    char *line = ALLOCATE(vm, char, currentSize);

    if (line == NULL) {
        runtimeError(vm, "Memory error on input()!");
        return NULL_VAL;
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
        return NULL_VAL;
    }
    
    if (argc != 0) {
        Value prompt = args[0];
        if (!IS_STRING(prompt)) {
            char *str = valueType(args[1]);
            runtimeError(vm, "Function getNumber() expected type 'string' but got '%s'.", str);
            free(str);
            
            return NULL_VAL;
        }
        
        printf("%s", AS_CSTRING(prompt));
    }
    
    uint64_t currentSize = 128;
    char *line = ALLOCATE(vm, char, currentSize);
    
    if (line == NULL) {
        runtimeError(vm, "Memory error on input()!");
        return NULL_VAL;
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

Value useIoLib(VM *vm) {
    ObjString *name = copyString(vm, "io", 2);
    push(vm, OBJ_VAL(name));
    ObjScript *lib = newScript(vm, name);
    push(vm, OBJ_VAL(lib));

    defineNative(vm, "input", ioInput, &lib->values);
    defineNative(vm, "getNumber", ioGetNumber, &lib->values);

    pop(vm);
    pop(vm);

    return OBJ_VAL(lib);
}
