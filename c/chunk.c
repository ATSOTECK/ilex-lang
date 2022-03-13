//
// Created by Skyler on 11/12/21.
//

#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk *chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lineCount = 0;
    chunk->lineCapacity = 0;
    chunk->lineInfo = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(LineInfo, chunk->lineInfo, chunk->lineCount);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    if (line >= chunk->lineCapacity) {
        int oldCapacity = chunk->lineCapacity;
        chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
        chunk->lineInfo = GROW_ARRAY(LineInfo, chunk->lineInfo, oldCapacity, chunk->lineCapacity);
    }

    chunk->code[chunk->count] = byte;

    chunk->lineInfo[line - 1].line = line;
    if (chunk->lineCount < line) {
        chunk->lineCount = line;
    }

    chunk->lineInfo[line - 1].offset = chunk->count;
    chunk->count++;
}

int addConstant(Chunk *chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

int getLine(Chunk *chunk, int offset) {
    for (int i = 0; i < chunk->lineCount; ++i) {
        if (offset <= chunk->lineInfo[i].offset) {
            return chunk->lineInfo[i].line;
        }
    }

    return -1;
}
