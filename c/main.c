#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ilex.h"

void printHelp() {
    printf("version -v ------ Print version number\n");
    printf("info    -i ------ Print build info\n");
    printf("help    -h ------ Print help text\n");
    printf("\n");
    printf("Usage: ilex [path]\n");
}

int main(int argc, char **argv) {
    if (argc >= 2) {
        if (strcmp("-v", argv[1]) == 0 || strcmp("version", argv[1]) == 0) {
            printf("%s\n", ILEX_VERSION);
            return 0;
        } else if (strcmp("-i", argv[1]) == 0 || strcmp("info", argv[1]) == 0) {
            printf("Ilex version %s built on %s.\n", ILEX_VERSION, ILEX_DATE);
            return 0;
        } else if (strcmp("-h", argv[1]) == 0 || strcmp("help", argv[1]) == 0) {
            printHelp();
            return 0;
        }
        
        VM *vm = initVM(argv[1], argc, argv);
        runFile(vm, argv[1]);
        freeVM(vm);
    } else {
        printHelp();
        exit(64);
    }

    return 0;
}
