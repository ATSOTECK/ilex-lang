//
// Created by Skyler on 5/2/22.
//

#include "lib_io.h"

#include "../memory.h"
#include "../vm.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef I_WIN
#   include <termios.h>
#   include <unistd.h>
#   define _fileno fileno
#else
#   include <io.h>
#endif

static Value ioInput(VM *vm, int argc, const Value *args) {
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

static Value ioGetNumber(VM *vm, int argc, const Value *args) {
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

static Value ioGetPass(VM *vm, int argc, const Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function getPass() expected 1 or 0 arguments but got %d", argc);
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

#ifndef I_WIN
    struct termios old, new;
    if (tcgetattr(fileno(stdin), &old) != 0) {
        runtimeError(vm, "Could not disable echo.");
        return ERROR_VAL;
    }

    new = old;
    new.c_lflag &= ~ECHO;
    if (tcsetattr(fileno(stdin), TCSAFLUSH, &new) != 0) {
        runtimeError(vm, "Could not disable echo.");
        return ERROR_VAL;
    }
#else
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
#endif

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

#ifndef I_WIN
    (void) tcsetattr(fileno(stdin), TCSAFLUSH, &old);
#else
    SetConsoleMode(hStdin, mode | ENABLE_ECHO_INPUT);
#endif

    printf("\n");

    return OBJ_VAL(takeString(vm, line, length));
}

static Value ioDisableEcho(VM *vm, const int argc, const Value *args) {
#ifndef I_WIN
    struct termios old, new;
    if (tcgetattr(fileno(stdin), &old) != 0) {
        runtimeError(vm, "Could not disable echo.");
        return ERROR_VAL;
    }

    new = old;
    new.c_lflag &= ~ECHO;
    if (tcsetattr(fileno(stdin), TCSAFLUSH, &new) != 0) {
        runtimeError(vm, "Could not disable echo.");
        return ERROR_VAL;
    }
#else
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
#endif
    
    return ZERO_VAL;
}

static Value ioEnableEcho(VM *vm, const int argc, const Value *args) {
#ifndef I_WIN
    struct termios ts;

    tcgetattr(fileno(stdin), &ts);
    ts.c_lflag |= ECHO;
    tcsetattr(fileno(stdin), TCSANOW, &ts);
#else
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode | ENABLE_ECHO_INPUT);
#endif
    
    return ZERO_VAL;
}

static Value ioIsATTY(VM *vm, int argc, const Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function isatty() expected 1 or 0 arguments but got %d", argc);
        return ERROR_VAL;
    }
    
    int fd;
    if (argc == 0) {
        fd = _fileno(stdout);
    } else if (IS_NUMBER(args[0])){
        fd = (int)AS_NUMBER(args[0]);
    } else {
        char *str = valueType(args[0]);
        runtimeError(vm, "Function isatty() expected type 'number' but got '%s'.", str);
        free(str);
        
        return ERROR_VAL;
    }
    
#ifdef I_WIN
    DWORD st;
    HANDLE h;
    
    if (!_isatty(fd)) {
        return FALSE_VAL;
    }
    
    h = (HANDLE)_get_osfhandle(fd);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE_VAL;
    }
    
    if (!GetConsoleMode(h, &st)) {
        return FALSE_VAL;
    }
    
    return TRUE_VAL;
#else
    return isatty(fd) ? TRUE_VAL : FALSE_VAL;
#endif
}

static Value ioFflush(VM *vm, int argc, const Value *args) {
    if (argc > 1) {
        runtimeError(vm, "Function fflush() expected 1 or 0 arguments but got %d", argc);
        return ERROR_VAL;
    }

    if (argc == 0) {
        fflush(stdout);
        return ZERO_VAL;
    }

    if (IS_NUMBER(args[0])) {
        switch ((int)AS_NUMBER(args[0])) {
            case 0: fflush(stdin); return ZERO_VAL;
            case 1: fflush(stdout); return ZERO_VAL;
            case 2: fflush(stderr); return ZERO_VAL;
            default: return ZERO_VAL;
        }
    }

    if (IS_NULL(args[0])) {
        fflush(NULL);
        return ZERO_VAL;
    }

    char *str = valueType(args[0]);
    runtimeError(vm, "Function fflush() expected type 'number' or 'null' but got '%s'.", str);
    free(str);

    return ERROR_VAL;
}

static Value ioFlushConsole(VM *vm, const int argc, const Value *args) {
    fflush(stdout);
    return ZERO_VAL;
}

static Value ioOpenFile(VM *vm, const int argc, const Value *args) {
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
    
    if (lib->used) {
        return OBJ_VAL(lib);
    }

    defineNative(vm, "input", ioInput, &lib->values);
    defineNative(vm, "getNumber", ioGetNumber, &lib->values);
    defineNative(vm, "getPass", ioGetPass, &lib->values);
    defineNative(vm, "echoOff", ioDisableEcho, &lib->values);
    defineNative(vm, "echoOn", ioEnableEcho, &lib->values);
    defineNative(vm, "isatty", ioIsATTY, &lib->values);
    
    defineNative(vm, "openFile", ioOpenFile, &lib->values);

    defineNative(vm, "fflush", ioFflush, &lib->values);
    defineNative(vm, "flushConsole", ioFlushConsole, &lib->values);
    defineNativeValue(vm, "stdin", NUMBER_VAL(0), &lib->values);
    defineNativeValue(vm, "stdout", NUMBER_VAL(1), &lib->values);
    defineNativeValue(vm, "stderr", NUMBER_VAL(2), &lib->values);

    pop(vm);
    pop(vm);
    
    lib->used = true;
    return OBJ_VAL(lib);
}
