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
    OP_GET_LOCAL,
    OP_GET_GLOBAL,
    OP_GET_UPVALUE,
    OP_GET_PROPERTY,
    OP_DEFINE_GLOBAL,
    OP_SET_LOCAL,
    OP_SET_GLOBAL,
    OP_SET_UPVALUE,
    OP_SET_PROPERTY,
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
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_RETURN,
    OP_CLASS,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t *code;
    int lineCount;
    int lineCapacity;
    int *lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);

#endif //C_CHUNK_H
