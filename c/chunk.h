//
// Created by Skyler on 11/12/21.
//

#ifndef C_CHUNK_H
#define C_CHUNK_H

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_EQ,
    OP_NOTEQ,
    OP_GR,
    OP_GREQ,
    OP_LT,
    OP_LTEQ,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NOT,
    OP_NEG,
    OP_PRINT,
    OP_RETURN,
} OpCode;

typedef struct {
    int line;
    int offset;
} LineInfo;

typedef struct {
    int count;
    int capacity;
    uint8_t *code;
    int lineCount;
    int lineCapacity;
    LineInfo *lineInfo;
    ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);
void writeConstant(Chunk *chunk, Value value, int line);

int getLine(Chunk *chunk, int offset);

#endif //C_CHUNK_H
