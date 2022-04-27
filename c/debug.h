//
// Created by Skyler on 11/12/21.
//

#ifndef C_DEBUG_H
#define C_DEBUG_H

#include "chunk.h"

void disassembleChunk(VM *vm, Chunk *chunk, const char *name);
int disassembleInstruction(VM *vm, Chunk *chunk, int offset);

#endif //C_DEBUG_H
