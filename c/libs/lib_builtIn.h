//
// Created by Skyler on 4/29/22.
//

#ifndef __C_LIB_BUILTIN_H__
#define __C_LIB_BUILTIN_H__

#include "../value.h"

#include "lib_math.h"

typedef Value (*BuiltInLib)(VM *vm);

typedef struct {
    char *name;
    BuiltInLib lib;
} BuiltInLibs;

Value useBuiltInLib(VM *vm, int idx);
int findBuiltInLib(char *name, int len);

#endif //__C_LIB_BUILTIN_H__
