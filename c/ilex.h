//
// Created by Skyler on 11/12/21.
//

#ifndef C_ILEX_H
#define C_ILEX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//#define DEBUG_PRINT_CODE
//#define DEBUG_TRACE_EXECUTION

//#define DEBUG_STRESS_GC
//#define DEBUG_LOG_GC

#define UINT8_COUNT (UINT8_MAX + 1)
#define UINT16_COUNT (UINT16_MAX + 1)

#define ILEX_VERSION_MAJOR 0
#define ILEX_VERSION_MINOR 0
#define ILEX_VERSION_BUILD 61
#define ILEX_VERSION "0.0.61"
#define ILEX_DATE "31 - August - 2022"

#if defined(WIN32) || defined(_WIN32)
#   define I_WIN
#   include <Windows.h>
#   define strtok_r strtok_s
#   define ssize_t SSIZE_T
#   define PATH_MAX 1024
#endif

#if defined(__APPLE__)
#   define I_MAC
#endif

#ifdef I_WIN
#   define PATH_SEPARATOR '\\'
#   define NORMAL_SEPARATOR '/'
#else
#   define PATH_SEPARATOR '/'
#endif

#ifdef NORMAL_SEPARATOR
#   define IS_SEP(x) ((x) == PATH_SEPARATOR || (x) == NORMAL_SEPARATOR)
#else
#   define IS_SEP(x) ((x) == PATH_SEPARATOR)
#endif

#define I_MAX_PATH 4096
#define I_ERR_MSG_SIZE 8192

typedef struct VM_ VM;
typedef uint64_t Value;

typedef enum {
    INTERPRET_GOOD          = (int)0x0B00B135,
    INTERPRET_COMPILE_ERROR = (int)0xBAADF00D,
    INTERPRET_RUNTIME_ERROR = (int)0xDEADDEAD,
    INTERPRET_ASSERT_ERROR  = (int)0xBAADC0DE,
    INTERPRET_PANIC_ERROR   = (int)0xBAAAAAAD
} InterpretResult;

typedef Value (*NativeFn)(VM *vm, int argCount, Value *args);
typedef Value (*BuiltInLib)(VM *vm);
typedef void (*ErrorCallback)(const char *msg);

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)

#define TAG_NULL  1 // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE  3 // 11.
#define TAG_ERR   4 // 10.

#define IS_BOOL(value)      (((value) | 1u) == TRUE_VAL)
#define IS_NULL(value)      ((value) == NULL_VAL)
#define IS_NUMBER(value)    (((value) & QNAN) != QNAN)
#define IS_ERR(value)       ((value) == ERROR_VAL)
#define IS_OBJ(value)       (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)      ((value) == TRUE_VAL)
#define AS_NUMBER(value)    valueToNum(value)
#define AS_OBJ(value)       ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))


#define BOOL_VAL(b)     ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL       ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL        ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NULL_VAL        ((Value)(uint64_t)(QNAN | TAG_NULL))
#define ERROR_VAL       ((Value)(uint64_t)(QNAN | TAG_ERR))
#define NUMBER_VAL(num) numToValue(num)
#define ZERO_VAL        numToValue(0)
#define OBJ_VAL(obj)    (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

typedef union {
    uint64_t bits64;
    uint32_t bits32[2];
    double num;
} IlexDouble;

static inline double valueToNum(Value value) {
    IlexDouble data;
    data.bits64 = value;
    return data.num;
}

static inline Value numToValue(double num) {
    IlexDouble data;
    data.num = num;
    return data.bits64;
}

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef struct {
    ObjString *key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry *entries;
} Table;

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
    OBJ_FILE,
    OBJ_MAP,
    OBJ_SET,
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

#if defined(__cplusplus)
extern "C" {
#endif

VM *initVM(const char *path);
void freeVM(VM *vm);
void runFile(VM *vm, const char *path);

void runtimeError(VM *vm, const char *format, ...);

void setRuntimeErrorCallback(VM *vm, ErrorCallback runtimeCallback);
void setAssertErrorCallback(VM *vm, ErrorCallback assertCallback);
void setPanicErrorCallback(VM *vm, ErrorCallback panicCallback);

ObjScript *newScript(VM *vm, ObjString* name);
ObjString *copyString(VM *vm, const char* chars, int length);

void registerGlobalFunction(VM *vm, const char *name, NativeFn function);
void registerGlobalValue(VM *vm, const char *name, Value value);

void registerLibraryFunction(VM *vm, const char *name, NativeFn function, Table *table);
void registerLibrary(VM *vm, const char *name, BuiltInLib lib);

//void registerType(VM *vm, const char *name); //Struct and conversion function
//void registerTypeFunction(VM *vm, const char *type, const char *name, NativeFn function);
//void registerTypeValue(VM *vm, const char *type, const char *name, Value value);
//TODO: Require function to free the type?

#if defined(__cplusplus)
}
#endif

#endif //C_ILEX_H
