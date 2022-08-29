//
// Created by Skyler on 05/21/22.
//

#ifndef __C_UTIL_H__
#define __C_UTIL_H__

#include "ilex.h"
#include "object.h"
#include "vm.h"

#include <stdio.h>

#ifdef I_WIN
ssize_t getline(char **restrict line, size_t *restrict n, FILE *restrict file);
#endif

bool resolvePath(const char *dir, const char *path, char *ret);
ObjString *dirName(VM *vm, const char *path, size_t len);
ObjString *getDir(VM *vm, const char *source);
char* readFile(const char *path);
bool isValidKey(Value value);

#endif //__C_UTIL_H__
