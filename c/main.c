#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ilex.h"

int runUnitTests(int argc, char **argv) {
    // recursively loop through the current directory and run any file that starts with 'test' and has '.ilex' file type
    // skip any file that ends with '_skipped'
    // in those files run every function that starts with 'test'
    // skip any function that ends in '_skipped'
    return 0;
}

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
        } else if (strcmp("test", argv[1]) == 0) {
            return runUnitTests(argc, argv);
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
