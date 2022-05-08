//
// Created by Skyler on 3/13/22.
//

#ifndef __C_OBJECT_H__
#define __C_OBJECT_H__

#include "ilex_include.h"
#include "chunk.h"
#include "common.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
#define IS_SCRIPT(value)       isObjType(value, OBJ_SCRIPT)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define IS_ENUM(value)         isObjType(value, OBJ_ENUM)
#define IS_ARRAY(value)        isObjType(value, OBJ_ARRAY)

#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value)        ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))
#define AS_SCRIPT(value)       ((ObjScript*)AS_OBJ(value))
#define AS_NATIVE(value)       (((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->str)
#define AS_ENUM(value)         ((ObjEnum*)AS_OBJ(value))
#define AS_ARRAY(value)        ((ObjArray*)AS_OBJ(value))

typedef enum {
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_SCRIPT,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE,
    OBJ_ENUM,
    OBJ_ARRAY,
} ObjType;

struct Obj {
    ObjType type;
    bool isMarked;
    struct Obj *next;
};

typedef struct {
    Obj obj;
    ObjString *name;
    ObjString *path;
    Table values;
} ObjScript;

typedef struct {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString *name;
} ObjFunction;

typedef Value (*NativeFn)(VM *vm, int argCount, Value *args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

struct ObjString {
    Obj obj;
    int len;
    char *str;
    uint32_t hash;
};

typedef struct ObjUpvalue {
    Obj obj;
    Value *location;
    Value closed;
    struct ObjUpvalue *next;
} ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction *function;
    ObjUpvalue **upvalues;
    int upvalueCount;
} ObjClosure;

typedef struct {
    Obj obj;
    ObjString *name;
    Table methods;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass *objClass;
    Table fields;
} ObjInstance;

typedef struct {
    Obj obj;
    Value receiver;
    ObjClosure *method;
} ObjBoundMethod;

typedef struct {
    Obj obj;
    ObjString *name;
    Table values;
} ObjEnum;

typedef struct {
    Obj obj;
    ValueArray data;
} ObjArray;

ObjBoundMethod *newBoundMethod(VM *vm, Value receiver, ObjClosure *method);
ObjClass *newClass(VM *vm, ObjString *name);
ObjClosure *newClosure(VM *vm, ObjFunction *function);
ObjFunction *newFunction(VM *vm);
ObjInstance *newInstance(VM *vm, ObjClass *objClass);
ObjScript *newScript(VM *vm, ObjString* name);
ObjNative *newNative(VM *vm, NativeFn function);
char *newCString(const char *str);
char *newCStringLen(const char *str, int len);
ObjString *takeString(VM *vm, char *str, int len);
ObjString *copyString(VM *vm, const char* chars, int length);
ObjUpvalue *newUpvalue(VM *vm, Value *slot);
ObjEnum *newEnum(VM *vm, ObjString *name);
ObjArray *newArray(VM *vm);
char *objectType(Value value);
char *objectToString(Value value);
char *arrayToString(ObjArray *array);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

static inline ObjType getObjType(Value value) {
    return AS_OBJ(value)->type;
}

#endif //__C_OBJECT_H__
