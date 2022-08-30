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
#define ILEX_VERSION_BUILD 60
#define ILEX_VERSION "0.0.60"
#define ILEX_DATE "29 - August - 2022"

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
extern "C" {            // Prevents name mangling of functions
#endif

VM *initVM(const char *path);
void freeVM(VM *vm);
void runFile(VM *vm, const char *path);

ObjScript *newScript(VM *vm, ObjString* name);
ObjString *copyString(VM *vm, const char* chars, int length);

void registerGlobalFunction(VM *vm, const char *name, NativeFn function);
void registerGlobalValue(VM *vm, const char *name, Value value);

void registerLibrary(VM *vm, const char *name, BuiltInLib lib);

//void registerType(VM *vm, const char *name); //Struct and conversion function
//void registerTypeFunction(VM *vm, const char *type, const char *name, NativeFn function);
//void registerTypeValue(VM *vm, const char *type, const char *name, Value value);
//TODO: Require function to free the type?

#if defined(__cplusplus)
}
#endif

#endif //C_ILEX_H
