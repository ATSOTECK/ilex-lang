//
// Created by Skyler on 05/21/22.
//

#ifndef __C_UTIL_H__
#define __C_UTIL_H__

#include "common.h"

#include <stdio.h>

#ifdef I_WIN
ssize_t getline(char **restrict line, size_t *restrict n, FILE *restrict file);
#endif

#endif //__C_UTIL_H__
