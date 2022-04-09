//
// Created by Skyler on 11/12/21.
//

#include <stdio.h>

#include "debug.h"
#include "value.h"

void disassembleChunk(Chunk *chunk, const char *name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");

    return offset + 2;
}

static int simpleInstruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int byteInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

int disassembleInstruction(Chunk *chunk, int offset) {
    printf("%04d ", offset);
    if (offset > 0 && getLine(chunk, offset) == getLine(chunk, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", getLine(chunk, offset));
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT: return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NULL: return simpleInstruction("OP_NULL", offset);
        case OP_TRUE: return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE: return simpleInstruction("OP_FALSE", offset);
        case OP_POP: return simpleInstruction("OP_POP", offset);
        case OP_GET_LOCAL: return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL: return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL: return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_SET_LOCAL: return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_SET_GLOBAL: return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_EQ: return simpleInstruction("OP_EQ", offset);
        case OP_NOTEQ: return simpleInstruction("OP_NOTEQ", offset);
        case OP_GR: return simpleInstruction("OP_GR", offset);
        case OP_GREQ: return simpleInstruction("OP_GREQ", offset);
        case OP_LT: return simpleInstruction("OP_LT", offset);
        case OP_LTEQ: return simpleInstruction("OP_LTEQ", offset);
        case OP_ADD: return simpleInstruction("OP_ADD", offset);
        case OP_SUB: return simpleInstruction("OP_SUB", offset);
        case OP_MUL: return simpleInstruction("OP_MUL", offset);
        case OP_DIV: return simpleInstruction("OP_DIV", offset);
        case OP_NOT: return simpleInstruction("OP_NOT", offset);
        case OP_NEG: return simpleInstruction("OP_NEG", offset);
        case OP_PRINT: return simpleInstruction("OP_PRINT", offset);
        case OP_RETURN: return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
