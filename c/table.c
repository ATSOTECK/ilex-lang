//
// Created by Skyler on 3/13/22.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

void initTable(Table *table) {
    table->count = 0;
    table->capacity = -1;
    table->entries = NULL;
}

void freeTable(VM *vm, Table *table) {
    FREE_ARRAY(vm, Entry, table->entries, table->capacity);
    initTable(table);
}

static Entry* findEntry(Entry *entries, int capacity, ObjString* key) {
    uint32_t index = key->hash & (capacity - 1);
    Entry *tombstone = NULL;

    for (;;) {
        Entry *entry = &entries[index];
        if (entry->key == NULL) {
            if (IS_NULL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) {
                    tombstone = entry;
                }
            }
        } else if (entry->key == key) {
            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}

static void adjustCapacity(VM *vm, Table* table, int capacity) {
    Entry *entries = ALLOCATE(vm, Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NULL_VAL;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL) {
            continue;
        }

        Entry *dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(vm, Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool tableGet(Table *table, ObjString *key, Value *value) {
    if (table->count == 0) {
        return false;
    }

    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return false;
    }

    *value = entry->value;
    return true;
}

bool tableSet(VM *vm, Table *table, ObjString *key, Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(vm, table, capacity);
    }

    Entry *entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    if (isNewKey && IS_NULL(entry->value)) {
        table->count++;
    }

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool tableDelete(Table *table, ObjString *key) {
    if (table->count == 0) {
        return false;
    }

    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return false;
    }

    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

void tableAddAll(VM *vm, Table *from, Table *to) {
    for (int i = 0; i < from->capacity; ++i) {
        Entry *entry = &from->entries[i];
        if (entry->key != NULL) {
            tableSet(vm, to, entry->key, entry->value);
        }
    }
}

ObjString *tableFindString(Table *table, const char *str, int len, uint32_t hash) {
    if (table->count == 0) {
        return NULL;
    }

    uint32_t index = hash & (table->capacity - 1);
    for (;;) {
        Entry *entry = &table->entries[index];
        if (entry->key == NULL) {
            if (IS_NULL(entry->value)) {
                return NULL;
            }
        } else if (entry->key->len == len &&
                   entry->key->hash == hash &&
                   memcmp(entry->key->str, str, len) == 0)
        {
            return entry->key;
        }

        index = (index + 1) & (table->capacity - 1);
    }
}

void tableRemoveWhite(Table *table) {
    for (int i = 0; i < table->capacity; ++i) {
        Entry *entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.isMarked) {
            tableDelete(table, entry->key);
        }
    }
}

void markTable(VM *vm, Table *table) {
    for (int i = 0; i < table->capacity; ++i) {
        Entry *entry = &table->entries[i];
        markObject(vm, (Obj*)entry->key);
        markValue(vm, entry->value);
    }
}

void printTable(Table *table) {
    for (int i = 0; i < table->capacity; ++i) {
        Entry entry = table->entries[i];
        if (entry.key == NULL) {
            printf("[NULL, ");
        } else {
            printf("[%s, ", entry.key->str);
        }
        printValue(entry.value);
        printf("]\n");
    }
}
