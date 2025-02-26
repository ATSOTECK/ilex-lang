//
// Created by Skyler on 3/13/22.
//

#ifndef __C_TABLE_H__
#define __C_TABLE_H__

#include "ilex.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75
#define TABLE_MIN_LOAD 0.25

void initTable(Table *table);
void freeTable(VM *vm, Table *table);
bool tableGet(const Table *table, ObjString *key, Value *value);
int tableGetKeyValue(const Table *table, char **key, Value *value, int startIndex);
bool tableSet(VM *vm, Table *table, ObjString *key, Value value, bool readOnly);
bool tableDelete(const Table *table, ObjString *key);
void tableAddAll(VM *vm, const Table *from, Table *to);
ObjString *tableFindString(const Table *table, const char *str, int len, uint32_t hash);

void tableRemoveWhite(Table *table);
void markTable(VM *vm, const Table *table);

void printTable(const Table *table);

#endif //__C_TABLE_H__
