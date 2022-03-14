//
// Created by Skyler on 3/13/22.
//

#ifndef __C_OBJECT_H__
#define __C_OBJECT_H__

#include "common.h"
#include "value.h"


#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_STRING(value)       isObjType(value, OBJ_STRING)

#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->str)

typedef enum {
    OBJ_STRING,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj *next;
};

struct ObjString {
    Obj obj;
    int len;
    char *str;
    uint32_t hash;
};

ObjString *takeString(char *str, int len);
ObjString *copyString(const char* chars, int length);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //__C_OBJECT_H__
