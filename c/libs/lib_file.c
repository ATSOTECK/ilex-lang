//
// Created by Skyler on 05/20/22.
//

#include "lib_file.h"

#include "../memory.h"
#include "../util.h"

#include <stdlib.h>

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
    
    if (file->file == NULL) {
        runtimeError(vm, "File is not open.");
        return ERROR_VAL;
    }
    
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
    
    if (file->file == NULL) {
        runtimeError(vm, "File is not open.");
        return ERROR_VAL;
    }
    
    if (strcmp(file->flags, "r") == 0) {
        runtimeError(vm, "File is not writeable.");
        return ERROR_VAL;
    }
    
    int written = fprintf(file->file, "%s\n", str->str);
    fflush(file->file);
    
    return NUMBER_VAL(written);
}

static Value fileRead(VM *vm, int argc, Value *args) {
    ObjFile *file = AS_FILE(args[0]);
    
    if (file->file == NULL) {
        runtimeError(vm, "File is not open.");
        return ERROR_VAL;
    }
    
    long int pos = ftell(file->file);
    fseek(file->file, 0, SEEK_END);
    long int size = ftell(file->file);
    fseek(file->file, pos, SEEK_SET);
    
    char *buf = ALLOCATE(vm, char, size + 1);
    if (buf == NULL) {
        runtimeError(vm, "Not enough memory to read '%s'.", file->path);
        return ERROR_VAL;
    }
    
    size_t bytesRead = fread(buf, sizeof(char), size + 1, file->file);
    if (bytesRead < size && !feof(file->file)) {
        FREE_ARRAY(vm, char, buf, size + 1);
        runtimeError(vm, "Could not read file '%s'.\n", file->path);
        return ERROR_VAL;
    }
    
    if (bytesRead != size) {
        buf = SHRINK_ARRAY(vm, buf, char, size + 1, bytesRead + 1);
    }
    
    buf[bytesRead] = '\0';
    return OBJ_VAL(takeString(vm, buf, bytesRead));
}

static Value fileReadln(VM *vm, int argc, Value *args) {
    ObjFile *file = AS_FILE(args[0]);
    
    if (file->file == NULL) {
        runtimeError(vm, "File is not open.");
        return ERROR_VAL;
    }
    
    char *line = NULL;
    size_t len = 0;
    if (getline(&line, &len, file->file) != -1) {
        if (line[len - 1] == '\n') {
            --len;
            line[len] = '\0';
        }
        
        return OBJ_VAL(copyString(vm, line, len));
    }
    
    return NULL_VAL;
}

static Value fileSeek(VM *vm, int argc, Value *args) {
    if (argc == 0 || argc > 2) {
        runtimeError(vm, "Function seek() expected 1 or 2 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    if (!IS_NUMBER(args[1])) {
        char *type = valueType(args[1]);
        runtimeError(vm, "Function seek() expected type 'number' for first argument but got '%s'.", type);
        free(type);
        
        return ERROR_VAL;
    }
    
    int whence = SEEK_SET;
    
    if (argc == 2) {
        if (!IS_NUMBER(args[2])) {
            char *type = valueType(args[2]);
            runtimeError(vm, "Function seek() expected type 'number' for second argument but got '%s'.", type);
            free(type);
        
            return ERROR_VAL;
        }
        
        int num = AS_NUMBER(args[2]);
        switch (num) {
            case 0: whence = SEEK_SET; break;
            case 1: whence = SEEK_CUR; break;
            case 2: whence = SEEK_END; break;
            default: whence = SEEK_SET;
        }
    }
    
    ObjFile *file = AS_FILE(args[0]);
    int offset = AS_NUMBER(args[1]);
    
    if (file->file == NULL) {
        runtimeError(vm, "File is not open.");
        return ERROR_VAL;
    }
    
    //TODO(Skyler): Better check for binary mode.
    if (offset != 0 && (file->flags[0] != 'b' && file->flags[1] != 'b')) {
        runtimeError(vm, "Function seek() may not have non-zero offset if file is opened in text mode.");
        return ERROR_VAL;
    }
    
    int ret = fseek(file->file, offset, whence);
    return NUMBER_VAL(ret);
}

static Value fileTell(VM *vm, int argc, Value *args) {
    ObjFile *file = AS_FILE(args[0]);
    
    if (file->file == NULL) {
        runtimeError(vm, "File is not open.");
        return ERROR_VAL;
    }
    
    long int ret = ftell(file->file);
    
    return NUMBER_VAL(ret);
}

static Value fileSize(VM *vm, int argc, Value *args) {
    ObjFile *file = AS_FILE(args[0]);
    
    if (file->file == NULL) {
        runtimeError(vm, "File is not open.");
        return ERROR_VAL;
    }
    
    long int pos = ftell(file->file);
    fseek(file->file, 0, SEEK_END);
    long int size = ftell(file->file);
    fseek(file->file, pos, SEEK_SET);
    
    return NUMBER_VAL(size);
}

static Value fileEmpty(VM *vm, int argc, Value *args) {
    ObjFile *file = AS_FILE(args[0]);
    
    if (file->file == NULL) {
        runtimeError(vm, "File is not open.");
        return ERROR_VAL;
    }
    
    long int pos = ftell(file->file);
    fseek(file->file, 0, SEEK_END);
    long int size = ftell(file->file);
    fseek(file->file, pos, SEEK_SET);
    
    return BOOL_VAL(size == 0);
}

static Value fileIsOpen(VM *vm, int argc, Value *args) {
    ObjFile *file = AS_FILE(args[0]);
    
    if (file->file == NULL) {
        return FALSE_VAL;
    }
    
    long int pos = ftell(file->file);
    
    return BOOL_VAL(pos >= 0);
}

static Value fileOpen(VM *vm, int argc, Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function open() expected 0 or 1 arguments but got '%d'.", argc);
        return ERROR_VAL;
    }
    
    ObjFile *file = AS_FILE(args[0]);
    
    if (argc == 1) {
        if (!IS_STRING(args[1])) {
            char *type = valueType(args[1]);
            runtimeError(vm, "Function open() expected type 'string' for first argument but got '%s'.", type);
            free(type);
        
            return ERROR_VAL;
        }
    
        ObjString *flagStr = AS_STRING(args[1]);
        file->flags = flagStr->str;
    }
    
#ifdef I_WIN
    errno_t err = fopen_s(&file->file, file->path, file->flags);
#else
    file->file = fopen(file->path, file->flags);
    errno_t err = file->file == NULL ? -1 : 0;
#endif
    
    if (err != 0) {
        runtimeError(vm, "Unable to open file '%s'.", file->path);
        return ERROR_VAL;
    }
    
    return ZERO_VAL;
}

static Value fileClose(VM *vm, int argc, Value *args) {
    ObjFile *file = AS_FILE(args[0]);
    
    if (file->file == NULL) {
        return ZERO_VAL;
    }
    
    int ret = fclose(file->file);
    file->file = NULL;
    
    return NUMBER_VAL(ret);
}

void defineFileFunctions(VM *vm) {
    defineNative(vm, "write", fileWrite, &vm->fileFunctions);
    defineNative(vm, "writeln", fileWriteLine, &vm->fileFunctions);
    defineNative(vm, "read", fileRead, &vm->fileFunctions);
    defineNative(vm, "readln", fileReadln, &vm->fileFunctions);
    
    defineNative(vm, "seek", fileSeek, &vm->fileFunctions);
    defineNative(vm, "tell", fileTell, &vm->fileFunctions);
    defineNative(vm, "size", fileSize, &vm->fileFunctions);
    defineNative(vm, "empty", fileEmpty, &vm->fileFunctions);
    defineNative(vm, "isOpen", fileIsOpen, &vm->fileFunctions);
    
    defineNative(vm, "open", fileOpen, &vm->fileFunctions);
    defineNative(vm, "close", fileClose, &vm->fileFunctions);
}
