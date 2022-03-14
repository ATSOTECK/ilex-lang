//
// Created by Skyler on 3/12/22.
//

#ifndef __C_COMPILER_H__
#define __C_COMPILER_H__

#include "object.h"
#include "vm.h"

bool compile(const char *source, Chunk *chunk);

#endif //__C_COMPILER_H__
