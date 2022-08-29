#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ilex.h"

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
