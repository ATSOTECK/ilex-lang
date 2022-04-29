#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void runFile(VM *vm, const char *path) {
    char *source = readFile(path);
    InterpretResult res = interpret(vm, source);
    free(source);

    switch (res) {
        case INTERPRET_COMPILE_ERROR: exit(99);
        case INTERPRET_RUNTIME_ERROR: exit(114);
        case INTERPRET_ASSERT_ERROR:  exit(97);
        case INTERPRET_PANIC_ERROR:   exit(112);
        case INTERPRET_GOOD:
        default: break;
    }
}

int main(int argc, char **argv) {
    VM *vm;
    if (argc == 2) {
        vm = initVM(argv[1]);
        runFile(vm, argv[1]);
    } else {
        fprintf(stderr, "Usage: ilex [path]\n");
        exit(64);
    }

    freeVM(vm);

    return 0;
}
