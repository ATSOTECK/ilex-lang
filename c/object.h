//
// Created by Skyler on 3/13/22.
//

#ifndef __C_OBJECT_H__
#define __C_OBJECT_H__

#include "ilex.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

ObjBoundMethod *newBoundMethod(VM *vm, Value receiver, ObjClosure *method);
ObjClass *newClass(VM *vm, ObjString *name, ObjClass *superclass, ClassType type);
ObjClosure *newClosure(VM *vm, ObjFunction *function);
ObjFunction *newFunction(VM *vm, FunctionType type, AccessLevel level, ObjScript *script);
ObjInstance *newInstance(VM *vm, ObjClass *objClass);
ObjNative *newNative(VM *vm, NativeFn function);
char *newCString(const char *str);
char *newCStringLen(const char *str, int len);
ObjString *takeString(VM *vm, char *str, int len);
ObjUpvalue *newUpvalue(VM *vm, Value *slot);
ObjEnum *newEnum(VM *vm, ObjString *name);
ObjArray *newArray(VM *vm);
ObjFile *newFile(VM *vm);
ObjMap *newMap(VM *vm);
ObjSet *newSet(VM *vm);
ObjAbstract *newAbstract(VM *vm, AbstractFreeFn freeFn);
char *objectType(Value value);
char *objectToString(Value value);
char *arrayToString(ObjArray *array);
char *mapToString(ObjMap *map);
char *setToString(ObjSet *set);
bool mapSet(VM *vm, ObjMap *map, Value key, Value value);
bool mapGet(const ObjMap *map, Value key, Value *value);
bool mapHasKey(const ObjMap *map, Value key);
bool mapDelete(VM *vm, ObjMap *map, Value key);
void mapClear(VM *vm, ObjMap *map);
void markMap(VM *vm, ObjMap *map);
ValueArray mapKeys(VM *vm, const ObjMap *map);
ValueArray mapValues(VM *vm, const ObjMap *map);
bool setAdd(VM *vm, ObjSet *set, Value value);
bool setGet(ObjSet *set, Value value);
bool setDelete(VM *vm, ObjSet *set, Value value);
void markSet(VM *vm, ObjSet *set);

ObjArray *copyArray(VM *vm, ObjArray *array, bool isShallow);
ObjMap *copyMap(VM *vm, ObjMap *map, bool isShallow);

#endif //__C_OBJECT_H__
