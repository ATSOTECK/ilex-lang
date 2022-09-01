//
// Created by Skyler on 11/12/21.
//

#ifndef C_CHUNK_H
#define C_CHUNK_H

#include "ilex.h"
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
    OP_GET_PROPERTY_NO_POP,
    OP_GET_SUPER,
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
    OP_INC,
    OP_SUB,
    OP_DEC,
    OP_MUL,
    OP_DIV,
    OP_POW,
    OP_MOD,
    OP_NOT,
    OP_BIT_AND,
    OP_BIT_OR,
    OP_BIT_XOR,
    OP_BIT_NOT,
    OP_BIT_LS,
    OP_BIT_RS,
    OP_NEG,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_JUMP_DO_WHILE,
    OP_LOOP,
    OP_CALL,
    OP_INVOKE,
    OP_SUPER_INVOKE,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_RETURN,
    OP_CLASS,
    OP_INHERIT,
    OP_METHOD,
    OP_ASSERT,
    OP_PANIC,
    OP_NULL_COALESCE,
    OP_OR,
    OP_MULTI_CASE,
    OP_CMP_JMP,
    OP_CMP_JMP_FALL,
    OP_ENUM,
    OP_ENUM_SET_VALUE,
    OP_USE,
    OP_USE_VAR,
    OP_USE_BUILTIN,
    OP_USE_BUILTIN_VAR,
    OP_USE_END,
    OP_BREAK,
    OP_NEW_ARRAY,
    OP_EMPTY,
    OP_SLICE,
    OP_INDEX,
    OP_INDEX_ASSIGN,
    OP_INDEX_PUSH,
    OP_OPEN_FILE,
    OP_CLOSE_FILE,
    OP_GET_SCRIPT,
    OP_SET_SCRIPT,
    OP_DEFINE_SCRIPT,
    OP_NEW_MAP,
    OP_NEW_SET,
} OpCode;

void initChunk(Chunk *chunk);
void freeChunk(VM *vm, Chunk *chunk);
void writeChunk(VM *vm, Chunk *chunk, uint8_t byte, int line);
int addConstant(VM *vm, Chunk *chunk, Value value);

#endif //C_CHUNK_H
