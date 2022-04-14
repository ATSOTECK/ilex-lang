//
// Created by Skyler on 3/12/22.
//

#ifndef __C_COMPILER_H__
#define __C_COMPILER_H__

#include "object.h"
#include "vm.h"

ObjFunction *compile(const char *source);
void markCompilerRoots();

#endif //__C_COMPILER_H__
