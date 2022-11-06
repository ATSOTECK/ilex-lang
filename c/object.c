//
// Created by Skyler on 3/13/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(vm, type, objectType) (type*)allocateObject(vm, sizeof(type), objectType)

static Obj* allocateObject(VM *vm, size_t size, ObjType type) {
    Obj *object = (Obj*)reallocate(vm, NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    object->next = vm->objects;
    vm->objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

    return object;
}

ObjBoundMethod *newBoundMethod(VM *vm, Value receiver, ObjClosure *method) {
    ObjBoundMethod *bound = ALLOCATE_OBJ(vm, ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClass *newClass(VM *vm, ObjString *name, ObjClass *superClass, ClassType type) {
    ObjClass* objClass = ALLOCATE_OBJ(vm, ObjClass, OBJ_CLASS);
    objClass->name = name;
    objClass->superClass = superClass;
    objClass->type = type;
    initTable(&objClass->methods);
    initTable(&objClass->privateMethods);
    initTable(&objClass->abstractMethods);
    initTable(&objClass->staticVars);
    initTable(&objClass->staticConsts);
    initTable(&objClass->fields);
    initTable(&objClass->privateFields);

    return objClass;
}

ObjClosure *newClosure(VM *vm, ObjFunction *function) {
    ObjUpvalue **upvalues = ALLOCATE(vm, ObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; ++i) {
        upvalues[i] = NULL;
    }

    ObjClosure *closure = ALLOCATE_OBJ(vm, ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;

    return closure;
}

ObjFunction *newFunction(VM *vm, FunctionType type, AccessLevel level, ObjScript *script) {
    ObjFunction* function = ALLOCATE_OBJ(vm, ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    function->type = type;
    function->accessLevel = level;
    function->script = script;
    initChunk(&function->chunk);

    return function;
}

ObjInstance *newInstance(VM *vm, ObjClass *objClass) {
    ObjInstance *instance = ALLOCATE_OBJ(vm, ObjInstance, OBJ_INSTANCE);
    instance->objClass = objClass;
    initTable(&instance->fields);
    initTable(&instance->privateFields);

    push(vm, OBJ_VAL(instance));
    ObjString *classStr = copyString(vm, "_class", 6);
    push(vm, OBJ_VAL(classStr));
    tableSet(vm, &instance->fields, classStr, OBJ_VAL(objClass));
    
    tableAddAll(vm, &objClass->fields, &instance->fields);
    tableAddAll(vm, &objClass->privateFields, &instance->privateFields);
    
    pop(vm);
    pop(vm);

    return instance;
}

ObjScript *newScript(VM *vm, ObjString* name) {
    Value moduleVal;
    if (tableGet(&vm->scripts, name, &moduleVal)) {
        return AS_SCRIPT(moduleVal);
    }

    ObjScript *script = ALLOCATE_OBJ(vm, ObjScript, OBJ_SCRIPT);
    initTable(&script->values);
    script->name = name;
    script->path = NULL;

    push(vm, OBJ_VAL(script));
    ObjString *file = copyString(vm, "$file", 5);
    push(vm, OBJ_VAL(file));

    tableSet(vm, &script->values, file, OBJ_VAL(name));
    tableSet(vm, &vm->scripts, name, OBJ_VAL(script));

    pop(vm);
    pop(vm);

    return script;
}

ObjNative *newNative(VM *vm, NativeFn function) {
    ObjNative* native = ALLOCATE_OBJ(vm, ObjNative, OBJ_NATIVE);
    native->function = function;

    return native;
}

static ObjString *allocateString(VM *vm, char *str, int len, uint32_t hash) {
    ObjString *string = ALLOCATE_OBJ(vm, ObjString, OBJ_STRING);
    string->len = len;
    string->str = str;
    string->hash = hash;

    push(vm, OBJ_VAL(string));
    tableSet(vm, &vm->strings, string, NULL_VAL);
    pop(vm);

    return string;
}

static uint32_t hashString(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }

    return hash;
}

char *newCString(const char *str) {
    size_t len = strlen(str);

    char* ret = (char*)malloc(sizeof(char) * (len + 1));
    memcpy(ret, str, len);
    ret[len] = '\0';

    return ret;
}

char *newCStringLen(const char *str, int len) {
    char* ret = (char*)malloc(sizeof(char) * (len + 1));
    memcpy(ret, str, len);
    ret[len] = '\0';

    return ret;
}

ObjString *takeString(VM *vm, char *str, int len) {
    uint32_t hash = hashString(str, len);
    ObjString *interned = tableFindString(&vm->strings, str, len, hash);
    if (interned != NULL) {
        FREE_ARRAY(vm, char, str, len + 1);
        return interned;
    }
    return allocateString(vm, str, len, hash);
}

ObjString *copyString(VM *vm, const char *str, int len) {
    uint32_t hash = hashString(str, len);
    ObjString *interned = tableFindString(&vm->strings, str, len, hash);
    if (interned != NULL) {
        return interned;
    }
    char *heapStr = ALLOCATE(vm, char, len + 1);
    memcpy(heapStr, str, len);
    heapStr[len] = '\0';

    return allocateString(vm, heapStr, len, hash);
}

ObjUpvalue *newUpvalue(VM *vm, Value *slot) {
    ObjUpvalue *upvalue = ALLOCATE_OBJ(vm, ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NULL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;

    return upvalue;
}

ObjEnum *newEnum(VM *vm, ObjString *name) {
    ObjEnum *enumObj = ALLOCATE_OBJ(vm, ObjEnum, OBJ_ENUM);
    enumObj->name = name;
    initTable(&enumObj->values);

    return enumObj;
}

ObjArray *newArray(VM *vm) {
    ObjArray *array = ALLOCATE_OBJ(vm, ObjArray, OBJ_ARRAY);
    initValueArray(&array->data);
    return array;
}

ObjFile *newFile(VM *vm) {
    return ALLOCATE_OBJ(vm, ObjFile, OBJ_FILE);
}

ObjMap *newMap(VM *vm) {
    ObjMap *map = ALLOCATE_OBJ(vm, ObjMap, OBJ_MAP);
    map->count = 0;
    map->capacity = -1;
    map->items = NULL;
    
    return map;
}

ObjSet *newSet(VM *vm) {
    ObjSet *set = ALLOCATE_OBJ(vm, ObjSet, OBJ_SET);
    set->count = 0;
    set->capacity = -1;
    set->items = NULL;

    return set;
}

static void printFunction(ObjFunction *function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }

    printf("<fn %s>", function->name->str);
}

//TODO(Skyler): The next 4 functions currently cause memory leaks.
static char *functionToString(ObjFunction *function) {
    if (function->name == NULL) {
        return newCString("<script>");
    }
    
    char *ret = (char*)malloc(sizeof(char) * function->name->len + 6);
    snprintf(ret, function->name->len + 6, "<fn %s>", function->name->str);
    
    return ret;
}

static char *instanceToString(ObjInstance *instance) {
    int len = 0, cap = 1024, written = 0;
    char *ret = (char *) malloc(sizeof(char) * cap);
    
    snprintf(ret, instance->objClass->name->len + 6, "<%s> { ", instance->objClass->name->str);
    len += instance->objClass->name->len + 5;
    written += len;

#define printVars(table)                                                \
    do {                                                                \
        idx = tableGetKeyValue(table, &key, &value, idx);               \
        if (idx == 0) {                                                 \
            done = true;                                                \
            break;                                                      \
        }                                                               \
                                                                        \
        int keyLen = (int) strlen(key);                                 \
        char *valStr = valueToString(value);                            \
        int valLen = (int) strlen(valStr);                              \
                                                                        \
        int size = keyLen + valLen + 4;                                 \
        len += size;                                                    \
        if (len + 10 >= cap) {                                          \
            ret = (char *) realloc(ret, cap + (sizeof(char) * 1024));   \
        }                                                               \
                                                                        \
        snprintf(ret + written, size + 1, "%s: %s, ", key, valStr);     \
        free(valStr);                                                   \
        written += size;                                                \
    } while(false);
    
    int idx = 0;
    char *key = NULL;
    bool done = false;
    Value value;
    while (!done) {
        printVars(&instance->fields);
    }
    
    snprintf(ret + written, 3, "| ");
    len += 2;
    written += 2;
    done = false;
    
    while (!done) {
        printVars(&instance->privateFields);
    }

#undef printVars
    
    snprintf(ret + written, 2, "}");
    
    return ret;
}

static char *scriptToString(ObjScript *script) {
    if (script->name == NULL) {
        return newCString("<library>");
    }
    
    char *ret = (char*)malloc(sizeof(char) * script->name->len + 11);
    snprintf(ret, script->name->len + 11, "<library %s>", script->name->str);
    
    return ret;
}

static char *enumToString(ObjEnum *objEnum) {
    char *enumString = malloc(sizeof(char) * (objEnum->name->len + 8));
    memcpy(enumString, "<enum ", 6);
    memcpy(enumString + 6, objEnum->name->str, objEnum->name->len);
    memcpy(enumString + 6 + objEnum->name->len, ">", 1);
    enumString[7 + objEnum->name->len] = '\0';

    return enumString;
}

char *arrayToString(ObjArray *array) {
    int size = 64;
    char *arrayStr = (char*)malloc(sizeof(char) * size);
    arrayStr[0] = '[';
    int len = 1;
    
    for (int i = 0; i < array->data.count; ++i) {
        Value value = array->data.values[i];
        
        char *elementStr;
        int elementSize;
        
        if (IS_STRING(value)) {
            ObjString *str = AS_STRING(value);
            elementStr = str->str;
            elementSize = str->len;
        } else {
            elementStr = valueToString(value);
            elementSize = (int)strlen(elementStr);
        }
        
        if (elementSize > (size - len - 6)) {
            if (elementSize > size) {
                size = elementSize * 2 + 6;
            } else {
                size = size * 2 + 6;
            }
    
            arrayStr = (char*)realloc(arrayStr, sizeof(char) * size);
        }
        
        if (IS_STRING(value)) {
            memcpy(arrayStr + len, "\"", 1);
            memcpy(arrayStr + len + 1, elementStr, elementSize);
            memcpy(arrayStr + len + 1 + elementSize, "\"", 1);
            len += elementSize + 2;
        } else {
            memcpy(arrayStr + len, elementStr, elementSize);
            len += elementSize;
            free(elementStr);
        }
    
        if (i != array->data.count - 1) {
            memcpy(arrayStr + len, ", ", 2);
            len += 2;
        }
    }
    
    arrayStr[len] = ']';
    arrayStr[len + 1] = '\0';
    
    return arrayStr;
}

static char *fileToString(ObjFile *file) {
    size_t len = strlen(file->path) + strlen(file->flags) + 11;
    char *fileStr = (char*)malloc(sizeof(char*) * len);
    snprintf(fileStr, len, "<file %s \"%s\">", file->path, file->flags);
    
    return fileStr;
}

char *mapToString(ObjMap *map) {
    int count = 0;
    int size = 64;
    char *mapStr = malloc(sizeof(char) * size);
    memcpy(mapStr, "{", 1);
    int strLen = 1;
    
    for (int i = 0; i <= map->capacity; ++i) {
        MapItem *item = &map->items[i];
        if (IS_ERR(item->key)) {
            continue;
        }
        
        ++count;
        
        char *key;
        int keySize;
        
        if (IS_STRING(item->key)) {
            ObjString *str = AS_STRING(item->key);
            key = str->str;
            keySize = str->len;
        } else {
            key = valueToString(item->key);
            keySize = (int)strlen(key);
        }
        
        if (keySize > (size - strLen - keySize - 4)) {
            if (keySize > size) {
                size += keySize * 2 + 4;
            } else {
                size *= 2 + 4;
            }
            
            char *buf = realloc(mapStr, sizeof(char) * size);
            
            if (buf == NULL) {
                printf("Out of memory!\n");
                exit(114);
            }
            
            mapStr = buf;
        }
        
        if (IS_STRING(item->key)) {
            memcpy(mapStr + strLen, "\"", 1);
            memcpy(mapStr + strLen + 1, key, keySize);
            memcpy(mapStr + strLen + 1 + keySize, "\": ", 3);
            strLen += keySize + 4;
        } else {
            memcpy(mapStr + strLen, key, keySize);
            memcpy(mapStr + strLen + keySize, ": ", 2);
            strLen += keySize + 2;
            free(key);
        }
        
        char *valueStr;
        int valueSize;
        
        if (IS_STRING(item->value)) {
            ObjString *str = AS_STRING(item->value);
            valueStr = str->str;
            valueSize = str->len;
        } else {
            valueStr = valueToString(item->value);
            valueSize = (int)strlen(valueStr);
        }
    
        if (valueSize > (size - strLen - valueSize - 6)) {
            if (valueSize > size) {
                size += valueSize * 2 + 6;
            } else {
                size *= 2 + 6;
            }
        
            char *buf = realloc(mapStr, sizeof(char) * size);
        
            if (buf == NULL) {
                printf("Out of memory!\n");
                exit(114);
            }
        
            mapStr = buf;
        }
        
        if (IS_STRING(item->value)) {
            memcpy(mapStr + strLen, "\"", 1);
            memcpy(mapStr + strLen + 1, valueStr, valueSize);
            memcpy(mapStr + strLen + 1 + valueSize, "\"", 1);
            strLen += valueSize + 2;
        } else {
            memcpy(mapStr + strLen, valueStr, valueSize);
            strLen += valueSize;
            free(valueStr);
        }
        
        if (count != map->count) {
            memcpy(mapStr + strLen, ", ", 2);
            strLen += 2;
        }
    }
    
    memcpy(mapStr + strLen, "}", 1);
    mapStr[strLen + 1] = '\0';
    
    return mapStr;
}

char *setToString(ObjSet *set) {
    int count = 0;
    int size = 64;
    char *setStr = malloc(sizeof(char) * size);
    memcpy(setStr, "{", 1);
    int strLen = 1;

    for (int i = 0; i <= set->capacity; ++i) {
        SetItem *item = &set->items[i];
        if (IS_ERR(item->value) || item->deleted) {
            continue;
        }

        ++count;

        char *valueStr;
        int valueSize;

        if (IS_STRING(item->value)) {
            ObjString *str = AS_STRING(item->value);
            valueStr = str->str;
            valueSize = str->len;
        } else {
            valueStr = valueToString(item->value);
            valueSize = (int) strlen(valueStr);
        }

        if (valueSize > (size - strLen - 5)) {
            if (valueSize > size * 2) {
                size += valueSize * 2 + 5;
            } else {
                size *= 2 + 5;
            }

            char *buf = realloc(setStr, sizeof(char) * size);

            if (buf == NULL) {
                printf("Out of memory!\n");
                exit(114);
            }

            setStr = buf;
        }

        if (IS_STRING(item->value)) {
            memcpy(setStr + strLen, "\"", 1);
            memcpy(setStr + strLen + 1, valueStr, valueSize);
            memcpy(setStr + strLen + 1 + valueSize, "\"", 1);
            strLen += valueSize + 2;
        } else {
            memcpy(setStr + strLen, valueStr, valueSize);
            strLen += valueSize;
            free(valueStr);
        }

        if (count != set->count) {
            memcpy(setStr + strLen, ", ", 2);
            strLen += 2;
        }
    }

    memcpy(setStr + strLen, "}", 1);
    setStr[strLen + 1] = '\0';

    return setStr;
}

static void adjustMapCapacity(VM *vm, ObjMap *map, int capacity) {
    MapItem *items = ALLOCATE(vm, MapItem, capacity + 1);
    for (int i = 0; i <= capacity; ++i) {
        items[i].key = ERROR_VAL;
        items[i].value = NULL_VAL;
        items[i].psl = 0;
    }
    
    MapItem *oldItems = map->items;
    int oldCapacity = map->capacity;
    
    map->count = 0;
    map->items = items;
    map->capacity = capacity;
    
    for (int i = 0; i <= oldCapacity; ++i) {
        MapItem *item = &oldItems[i];
        if (IS_ERR(item->key)) {
            continue;
        }
    
        mapSet(vm, map, item->key, item->value);
    }
    
    FREE_ARRAY(vm, MapItem, oldItems, oldCapacity + 1);
}

bool mapSet(VM *vm, ObjMap *map, Value key, Value value) {
    if (map->count + 1 > (map->capacity + 1) * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(map->capacity + 1) - 1;
        adjustMapCapacity(vm, map, capacity);
    }
    
    uint32_t index = hashValue(key) & map->capacity;
    MapItem *bucket;
    bool isNewKey = false;
    
    MapItem item;
    item.key = key;
    item.value = value;
    item.psl = 0;
    
    for (;;) {
        bucket = &map->items[index];
        
        if (IS_ERR(bucket->key)) {
            isNewKey = true;
            break;
        } else {
            if (valuesEqual(key, bucket->key)) {
                break;
            }
            
            if (item.psl > bucket->psl) {
                isNewKey = true;
                MapItem tmp = item;
                item = *bucket;
                *bucket = tmp;
            }
        }
        
        index = (index + 1) & map->capacity;
        ++item.psl;
    }
    
    *bucket = item;
    if (isNewKey) {
        ++map->count;
    }
    
    return isNewKey;
}

bool mapGet(ObjMap *map, Value key, Value *value) {
    if (map->count == 0) {
        return false;
    }
    
    MapItem *item;
    uint32_t index = hashValue(key) & map->capacity;
    uint32_t psl = 0;
    
    for (;;) {
        item = &map->items[index];
        
        if (IS_ERR(item->key) || psl > item->psl) {
            return false;
        }
        
        if (valuesEqual(key, item->key)) {
            break;
        }
        
        index = (index + 1) & map->capacity;
        ++psl;
    }
    
    *value = item->value;
    return true;
}

bool mapDelete(VM *vm, ObjMap *map, Value key) {
    if (map->count == 0) {
        return false;
    }
    
    int capacity = map->capacity;
    uint32_t index = hashValue(key) & capacity;
    uint32_t psl = 0;
    MapItem *item;
    
    for (;;) {
        item = &map->items[index];
        
        if (IS_ERR(key) || psl > item->psl) {
            return false;
        }
        
        if (valuesEqual(key, item->key)) {
            break;
        }
        
        index = (index + 1) & capacity;
        ++psl;
    }
    
    --map->count;
    
    for (;;) {
        MapItem *nextItem;
        item->key = ERROR_VAL;
        item->value = ERROR_VAL;
        
        index = (index + 1) & capacity;
        nextItem = &map->items[index];
        
        // Stop if we hit an empty bucket or hit a key which is in its original location.
        if (IS_ERR(nextItem->key) || nextItem->psl == 0) {
            break;
        }
        
        --nextItem->psl;
        *item = *nextItem;
        item = nextItem;
    }
    
    if (map->count - 1 < map->capacity * TABLE_MIN_LOAD) {
        capacity = SHRINK_CAPACITY(map->capacity + 1) - 1;
        adjustMapCapacity(vm, map, capacity);
    }
    
    return true;
}

void markMap(VM *vm, ObjMap *map) {
    for (int i = 0; i <= map->capacity; ++i) {
        MapItem *item = &map->items[i];
        markValue(vm, item->key);
        markValue(vm, item->value);
    }
}

static SetItem *setFindItem(SetItem *items, int capacity, Value value) {
    uint32_t index = hashValue(value) & capacity;
    SetItem *tombstone = NULL;
    
    for (;;) {
        SetItem *item = &items[index];
        
        if (IS_ERR(item->value)) {
            if (!item->deleted) {
                return tombstone != NULL ? tombstone : item;
            } else {
                if (tombstone == NULL) {
                    tombstone = item;
                }
            }
        } else if (valuesEqual(value, item->value)) {
            return item;
        }
        
        index = (index + 1) & capacity;
    }
}

static void adjustSetCapacity(VM *vm, ObjSet *set, int capacity) {
    SetItem *items = ALLOCATE(vm, SetItem, capacity + 1);
    for (int i = 0; i <= capacity; ++i) {
        items[i].value = ERROR_VAL;
        items[i].deleted = false;
    }
    
    set->count = 0;
    
    for (int i = 0; i <= set->capacity; ++i) {
        SetItem *item = &set->items[i];
        if (IS_ERR(item->value) || item->deleted) {
            continue;
        }
        
        SetItem *dst = setFindItem(items, capacity, item->value);
        dst->value = item->value;
        ++set->count;
    }
    
    FREE_ARRAY(vm, SetItem, set->items, set->capacity + 1);
    set->items = items;
    set->capacity = capacity;
}

bool setAdd(VM *vm, ObjSet *set, Value value) {
    if (set->count + 1 > (set->capacity + 1) * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(set->capacity + 1) - 1;
        adjustSetCapacity(vm, set, capacity);
    }
    
    SetItem *item = setFindItem(set->items, set->capacity, value);
    bool isNewValue = IS_ERR(item->value) || item->deleted;
    item->value = value;
    item->deleted = false;
    
    if (isNewValue) {
        ++set->count;
    }
    
    return isNewValue;
}

bool setGet(ObjSet *set, Value value) {
    if (set->count == 0) {
        return false;
    }
    
    SetItem *item = setFindItem(set->items, set->capacity, value);
    return !(IS_ERR(item->value) || item->deleted);
}

bool setDelete(VM *vm, ObjSet *set, Value value) {
    if (set->count == 0) {
        return false;
    }
    
    SetItem *item = setFindItem(set->items, set->capacity, value);
    if (IS_ERR(item->value)) {
        return false;
    }
    
    --set->count;
    item->deleted = true;
    
    if (set->count - 1 < set->capacity * TABLE_MIN_LOAD) {
        int capacity = SHRINK_CAPACITY(set->capacity + 1) - 1;
        adjustSetCapacity(vm, set, capacity);
    }
    
    return true;
}

void markSet(VM *vm, ObjSet *set) {
    for (int i = 0; i <= set->capacity; ++i) {
        SetItem *item = &set->items[i];
        markValue(vm, item->value);
    }
}

ObjArray *copyArray(VM *vm, ObjArray *array, bool isShallow) {
    ObjArray *ret = newArray(vm);
    push(vm, OBJ_VAL(ret));
    
    for (int i = 0; i < array->data.count; ++i) {
        Value val = array->data.values[i];
        
        if (!isShallow) {
            if (IS_ARRAY(val)) {
                val = OBJ_VAL(copyArray(vm, AS_ARRAY(val), false));
            } else if (IS_MAP(val)) {
                val = OBJ_VAL(copyMap(vm, AS_MAP(val), false));
            }
            // TODO: Instance
        }
    
        push(vm, val);
        writeValueArray(vm, &ret->data, val);
        pop(vm);
    }
    
    pop(vm);
    return ret;
}

ObjMap *copyMap(VM *vm, ObjMap *map, bool isShallow) {
    ObjMap *ret = newMap(vm);
    push(vm, OBJ_VAL(ret));
    
    for (int i = 0; i < ret->capacity; ++i) {
        if (IS_ERR(map->items[i].key)) {
            continue;
        }
        
        Value val = map->items[i].value;
        
        if (!isShallow) {
            if (IS_ARRAY(val)) {
                val = OBJ_VAL(copyArray(vm, AS_ARRAY(val), false));
            } else if (IS_MAP(val)) {
                val = OBJ_VAL(copyMap(vm, AS_MAP(val), false));
            }
            // TODO: Instance
        }
        
        push(vm, val);
        mapSet(vm, ret, map->items[i].key, val);
        pop(vm);
    }
    
    pop(vm);
    return ret;
}

char *objectType(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD: return newCString("boundMethod");
        case OBJ_FUNCTION: return newCString("function");
        case OBJ_CLOSURE: return newCString("closure");
        case OBJ_CLASS: return newCString("class");
        case OBJ_INSTANCE: {
            ObjInstance *instance = AS_INSTANCE(value);
            return newCString(instance->objClass->name->str);
        }
        case OBJ_SCRIPT: return newCString("script");
        case OBJ_NATIVE: return newCString("nativeFunction");
        case OBJ_STRING: return newCString("string");
        case OBJ_UPVALUE: return newCString("upvalue");
        case OBJ_ENUM: return newCString("enum");
        case OBJ_ARRAY: return newCString("array");
        case OBJ_FILE: return newCString("file");
        case OBJ_MAP: return newCString("map");
        case OBJ_SET: return newCString("set");
    }

    return newCString("unknown type");
}

char *objectToString(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD: return functionToString(AS_BOUND_METHOD(value)->method->function);
        case OBJ_CLASS: return newCString(AS_CLASS(value)->name->str);
        case OBJ_CLOSURE: return functionToString(AS_CLOSURE(value)->function);
        case OBJ_FUNCTION: return functionToString(AS_FUNCTION(value));
        case OBJ_INSTANCE: return instanceToString(AS_INSTANCE(value));
        case OBJ_SCRIPT: return scriptToString(AS_SCRIPT(value));
        case OBJ_NATIVE: return newCString("<native fn>");
        case OBJ_STRING: return newCString(AS_STRING(value)->str);
        case OBJ_UPVALUE: return newCString("Should never happen.");
        case OBJ_ENUM: return enumToString(AS_ENUM(value));
        case OBJ_ARRAY: return arrayToString(AS_ARRAY(value));
        case OBJ_FILE: return fileToString(AS_FILE(value));
        case OBJ_MAP: return mapToString(AS_MAP(value));
        case OBJ_SET: return setToString(AS_SET(value));
    }
    
    return newCString("unknown object");
}
