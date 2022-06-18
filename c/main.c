#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "util.h"
#include "vm.h"

static void runFile(VM *vm, const char *path) {
    char *source = readFile(path);
    InterpretResult res = interpret(vm, path, source);
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
    if (argc == 2) {
        if (strcmp("-v", argv[1]) == 0) {
            printf("%s\n", ILEX_VERSION);
            return 0;
        } else if (strcmp("-V", argv[1]) == 0) {
            printf("Ilex version %s built on %s.\n", ILEX_VERSION, ILEX_DATE);
            return 0;
        }
        
        VM *vm = initVM(argv[1]);
        runFile(vm, argv[1]);
        freeVM(vm);
    } else {
        fprintf(stderr, "Usage: ilex [path]\n");
        exit(64);
    }

    return 0;
}
