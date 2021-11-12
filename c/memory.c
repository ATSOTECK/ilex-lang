//
// Created by Skyler on 11/12/21.
//

#include <stdlib.h>

#include "memory.h"

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void *mem = realloc(pointer, newSize);
    if (mem == NULL) {
        exit(69);
    }

    return mem;
}
