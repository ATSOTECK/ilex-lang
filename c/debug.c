//
// Created by Skyler on 11/12/21.
//

#include <stdio.h>

#include "debug.h"
#include "object.h"
#include "value.h"

void disassembleChunk(Chunk *chunk, const char *name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

// TODO: There is a bug here. Causes a crash when printing the value with OP_SET_CLASS_STATIC_VAR.
static int constantInstruction(const char *name, Chunk *chunk, int offset) {
    uint16_t constant = (uint16_t)(chunk->code[offset + 1] << 8);
    constant |= chunk->code[offset + 2];

    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");

    return offset + 3;
}

static int invokeInstruction(const char *name, Chunk *chunk, int offset) {
    uint16_t constant = (uint16_t)(chunk->code[offset + 1] << 8);
    constant |= chunk->code[offset + 2];
    uint8_t argCount = chunk->code[offset + 3];
    printf("%-16s (%d args) %4d '", name, argCount, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");

    return offset + 4;
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

static int shortInstruction(const char *name, Chunk *chunk, int offset) {
    uint16_t slot = (uint16_t)(chunk->code[offset + 1] << 8);
    slot |= chunk->code[offset + 2];
    printf("%-16s %4d\n", name, slot);
    return offset + 3;
}

static int jumpInstruction(const char *name, int sign, Chunk *chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static int useBuiltinInstruction(const char* name, Chunk* chunk, int offset) {
    uint16_t lib = (uint16_t)(chunk->code[offset + 2] << 8);
    lib |= chunk->code[offset + 3];
    printf("%-18s '", name);
    printValue(chunk->constants.values[lib]);
    printf("'\n");
    return offset + 4;
}

static int useFromBuiltinInstruction(const char* name, Chunk* chunk, int offset) {
    uint16_t lib = (uint16_t)(chunk->code[offset + 1] << 8);
    lib |= chunk->code[offset + 2];
    uint8_t argc = chunk->code[offset + 3];
    printf("%-18s '", name);
    printValue(chunk->constants.values[lib]);
    printf("'\n");
    return offset + 5 + argc;
}

static int classInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t type = chunk->code[offset + 1];
    uint16_t constant = (uint16_t)(chunk->code[offset + 2] << 8);
    constant |= chunk->code[offset + 3];
    char *typeStr;

    switch (type) {
        case CLASS_DEFAULT: typeStr = "default"; break;
        case CLASS_ABSTRACT: typeStr = "abstract"; break;
        case CLASS_BEHAVIOR: typeStr = "behavior"; break;
        default: typeStr = "default"; break;
    }

    printf("%-19s Type: %s %4d '", name, typeStr, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 4;
}

int disassembleInstruction(Chunk *chunk, int offset) {
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT: return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NULL: return simpleInstruction("OP_NULL", offset);
        case OP_EMPTY: return simpleInstruction("OP_EMPTY", offset);
        case OP_TRUE: return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE: return simpleInstruction("OP_FALSE", offset);
        case OP_POP: return simpleInstruction("OP_POP", offset);
        case OP_GET_LOCAL: return shortInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL: return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_GET_UPVALUE: return shortInstruction("OP_GET_UPVALUE", chunk, offset);
        case OP_GET_PROPERTY: return constantInstruction("OP_GET_PROPERTY", chunk, offset);
        case OP_GET_PROPERTY_NO_POP: return constantInstruction("OP_GET_PROPERTY_NO_POP", chunk, offset);
        case OP_GET_PRIVATE_PROPERTY: return constantInstruction("OP_GET_PRIVATE_PROPERTY", chunk, offset);
        case OP_GET_PRIVATE_PROPERTY_NO_POP: return constantInstruction("OP_GET_PRIVATE_PROPERTY_NO_POP", chunk, offset);
        case OP_GET_SUPER: return constantInstruction("OP_GET_SUPER", chunk, offset);
        case OP_GET_SCRIPT: return constantInstruction("OP_GET_SCRIPT", chunk, offset);
        case OP_DEFINE_GLOBAL: return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_DEFINE_SCRIPT: return constantInstruction("OP_DEFINE_SCRIPT", chunk, offset);
        case OP_SET_LOCAL: return shortInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_SET_GLOBAL: return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_SET_UPVALUE: return shortInstruction("OP_SET_UPVALUE", chunk, offset);
        case OP_SET_PROPERTY: return constantInstruction("OP_SET_PROPERTY", chunk, offset);
        case OP_SET_SCRIPT: return constantInstruction("OP_SET_SCRIPT", chunk, offset);
        case OP_SET_PRIVATE_PROPERTY: return constantInstruction("OP_SET_PRIVATE_PROPERTY", chunk, offset);
        case OP_SET_CLASS_STATIC_VAR: return constantInstruction("OP_SET_CLASS_STATIC_VAR", chunk, offset);
        case OP_EQ: return simpleInstruction("OP_EQ", offset);
        case OP_NOTEQ: return simpleInstruction("OP_NOTEQ", offset);
        case OP_GR: return simpleInstruction("OP_GR", offset);
        case OP_GREQ: return simpleInstruction("OP_GREQ", offset);
        case OP_LT: return simpleInstruction("OP_LT", offset);
        case OP_LTEQ: return simpleInstruction("OP_LTEQ", offset);
        case OP_ADD: return simpleInstruction("OP_ADD", offset);
        case OP_INC: return simpleInstruction("OP_INC", offset);
        case OP_SUB: return simpleInstruction("OP_SUB", offset);
        case OP_DEC: return simpleInstruction("OP_DEC", offset);
        case OP_MUL: return simpleInstruction("OP_MUL", offset);
        case OP_DIV: return simpleInstruction("OP_DIV", offset);
        case OP_POW: return simpleInstruction("OP_POW", offset);
        case OP_MOD: return simpleInstruction("OP_MOD", offset);
        case OP_NULL_COALESCE: return simpleInstruction("OP_OP_NULL_COALESCE", offset);
        case OP_NOT: return simpleInstruction("OP_NOT", offset);
        case OP_NEG: return simpleInstruction("OP_NEG", offset);
        case OP_JUMP: return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE: return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_JUMP_DO_WHILE: return jumpInstruction("OP_JUMP_DO_WHILE", -1, chunk, offset);
        case OP_LOOP: return jumpInstruction("OP_LOOP", -1, chunk, offset);
        case OP_CALL: return byteInstruction("OP_CALL", chunk, offset);
        case OP_INVOKE: return invokeInstruction("OP_INVOKE", chunk, offset);
        case OP_INVOKE_SUPER: return invokeInstruction("OP_SUPER_INVOKE", chunk, offset);
        case OP_INVOKE_THIS: return invokeInstruction("OP_INVOKE_THIS", chunk, offset);
        case OP_CLOSURE: {
            offset++;
            uint16_t constant = (uint16_t)(chunk->code[offset] << 8);
            constant |= chunk->code[offset + 1];
            offset += 2;
            printf("%-16s %4d ", "OP_CLOSURE", constant);
            printValue(chunk->constants.values[constant]);
            printf("\n");

            ObjFunction *function = AS_FUNCTION(chunk->constants.values[constant]);
            for (int i = 0; i < function->upvalueCount; ++i) {
                int isLocal = chunk->code[offset++];
                int index = (int)(chunk->code[offset++] << 8);
                index |= chunk->code[offset++ + 1];
                printf("%04d      |                     %s %d\n", offset - 2, isLocal ? "local" : "upvalue", index);
            }

            return offset;
        }
        case OP_CLOSE_UPVALUE: return simpleInstruction("OP_CLOSE_UPVALUE", offset);
        case OP_RETURN: return simpleInstruction("OP_RETURN", offset);
        case OP_CLASS: return classInstruction("OP_CLASS", chunk, offset);
        case OP_INHERIT: return classInstruction("OP_INHERIT", chunk, offset);
        case OP_CHECK_ABSTRACT: return simpleInstruction("OP_CHECK_ABSTRACT", offset);
        case OP_METHOD: return constantInstruction("OP_METHOD", chunk, offset);
        case OP_ASSERT: return constantInstruction("OP_ASSERT", chunk, offset);
        case OP_NEW_ARRAY: return byteInstruction("OP_NEW_ARRAY", chunk, offset);
        case OP_NEW_MAP: return byteInstruction("OP_NEW_MAP", chunk, offset);
        case OP_USE: return constantInstruction("OP_USE", chunk, offset);
        case OP_USE_VAR: return simpleInstruction("OP_USE_VAR", offset);
        case OP_USE_BUILTIN: return useBuiltinInstruction("OP_USE_BUILTIN", chunk, offset);
        case OP_USE_BUILTIN_VAR: return useFromBuiltinInstruction("OP_USE_BUILTIN_VAR", chunk, offset);
        case OP_USE_END: return simpleInstruction("OP_USE_END", offset);
        case OP_MULTI_CASE: return byteInstruction("OP_MULTI_CASE", chunk, offset);
        case OP_CMP_JMP: return jumpInstruction("OP_CMP_JMP", 1, chunk, offset);
        case OP_CMP_JMP_FALL: return jumpInstruction("OP_CMP_JMP_FALL", 1, chunk, offset);
        case OP_INDEX: return simpleInstruction("OP_INDEX", offset);
        case OP_INDEX_ASSIGN: return simpleInstruction("OP_INDEX_ASSIGN", offset);
        case OP_INDEX_PUSH: return simpleInstruction("OP_INDEX_PUSH", offset);
        case OP_SLICE: return simpleInstruction("OP_SLICE", offset);
        case OP_OPEN_FILE: return constantInstruction("OP_OPEN_FILE", chunk, offset);
        case OP_CLOSE_FILE: return constantInstruction("OP_CLOSE_FILE", chunk, offset);
        case OP_DEFINE_DEFAULT: return constantInstruction("OP_DEFINE_DEFAULT", chunk, offset);
        case OP_ENUM: return constantInstruction("OP_ENUM", chunk, offset);
        case OP_ENUM_SET_VALUE: return constantInstruction("OP_ENUM_SET_VALUE", chunk, offset);
        default:
            printf("??? Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
