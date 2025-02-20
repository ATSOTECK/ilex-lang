//
// Created by Skyler Burwell on 2/19/25.
//

#ifndef __C_LIB_HTTP_H__
#define __C_LIB_HTTP_H__

#include "../vm.h"

typedef struct {
    VM *vm;
    ObjMap *headers;
    char *res;
    size_t len;
    ObjString *status;
    int statusCode;
    bool firstIteration;
} Response;

Value useHttpLib(VM *vm);

#endif //__C_LIB_HTTP_H__
