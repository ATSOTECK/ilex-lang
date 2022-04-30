//
// Created by Skyler on 11/12/21.
//

#ifndef C_DEBUG_H
#define C_DEBUG_H

#include "chunk.h"

void disassembleChunk(Chunk *chunk, const char *name);
int disassembleInstruction(Chunk *chunk, int offset);

#endif //C_DEBUG_H
