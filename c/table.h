//
// Created by Skyler on 3/13/22.
//

#ifndef __C_TABLE_H__
#define __C_TABLE_H__

#include "common.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75
#define TABLE_MIN_LOAD 0.25

typedef struct {
    ObjString *key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry *entries;
} Table;

void initTable(Table *table);
void freeTable(VM *vm, Table *table);
bool tableGet(Table *table, ObjString *key, Value *value);
bool tableSet(VM *vm, Table *table, ObjString *key, Value value);
bool tableDelete(Table *table, ObjString *key);
void tableAddAll(VM *vm, Table *from, Table *to);
ObjString *tableFindString(Table *table, const char *str, int len, uint32_t hash);

void tableRemoveWhite(Table *table);
void markTable(VM *vm, Table *table);

void printTable(Table *table);

#endif //__C_TABLE_H__
