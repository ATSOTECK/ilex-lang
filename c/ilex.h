//
// Created by Skyler on 11/12/21.
//

#ifndef C_ILEX_H
#define C_ILEX_H

#ifndef __cplusplus
#   include <stdbool.h>
#   include <stddef.h>
#   include <stdint.h>
#   include <time.h>
#else
#   include <cstdint>
#   include <ctime>
#endif

#include <stdio.h>

#ifndef __cplusplus
#   define nullptr ((void*)0)
#endif

//#define DEBUG_PRINT_CODE
//#define DEBUG_TRACE_EXECUTION

//#define DEBUG_STRESS_GC
//#define DEBUG_LOG_GC

//#define DEBUG_MODE

#define UINT8_COUNT (UINT8_MAX + 1)
#define UINT16_COUNT (UINT16_MAX + 1)

#define ILEX_VERSION_MAJOR 0
#define ILEX_VERSION_MINOR 0
#define ILEX_VERSION_BUILD 66
#define ILEX_VERSION "0.0.66"
#define ILEX_DATE "23 - November - 2023"

#if defined(WIN32) || defined(_WIN32)
#   define I_WIN
#   include <Windows.h>
#   define strtok_r strtok_s
#   define ssize_t SSIZE_T
#   ifndef PATH_MAX
#       define PATH_MAX 1024
#   endif
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

#define WINDOW_WIDTH_DEFAULT 1200
#define WINDOW_HEIGHT_DEFAULT 720

typedef struct VM_ VM;
typedef uint64_t Value;

#define ILEX_READ_ONLY true
#define ILEX_READ_WRITE false

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

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)
#define IS_DEFAULT_CLASS(value)   isObjType(value, OBJ_CLASS) && AS_CLASS(value)->type == CLASS_DEFAULT
#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
#define IS_SCRIPT(value)       isObjType(value, OBJ_SCRIPT)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define IS_ENUM(value)         isObjType(value, OBJ_ENUM)
#define IS_ARRAY(value)        isObjType(value, OBJ_ARRAY)
#define IS_FILE(value)         isObjType(value, OBJ_FILE)
#define IS_MAP(value)          isObjType(value, OBJ_MAP)
#define IS_SET(value)          isObjType(value, OBJ_SET)
#define IS_ABSTRACT(value)     isObjType(value, OBJ_ABSTRACT)
#define IS_WINDOW(value)       isObjType(value, OBJ_WINDOW)

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
#define AS_FILE(value)         ((ObjFile*)AS_OBJ(value))
#define AS_MAP(value)          ((ObjMap*)AS_OBJ(value))
#define AS_SET(value)          ((ObjSet*)AS_OBJ(value))
#define AS_ABSTRACT(value)     ((ObjAbstract*)AS_OBJ(value))
#define AS_WINDOW(value)       ((ObjWindow*)AS_OBJ(value))

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
    OBJ_ABSTRACT,

    OBJ_WINDOW,
} ObjType;

typedef enum {
    CLASS_DEFAULT,
    CLASS_ABSTRACT,
    CLASS_BEHAVIOR,
} ClassType;

typedef enum {
    ACCESS_PUBLIC,
    ACCESS_PRIVATE,
} AccessLevel;

typedef enum {
    TYPE_FUNCTION,
    TYPE_ANON_FUNCTION,
    TYPE_CONSTRUCTOR,
    TYPE_METHOD,
    TYPE_STATIC,
    TYPE_ABSTRACT,
    TYPE_TOP_LEVEL,
} FunctionType;

typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

typedef struct {
    int count;
    int capacity;
    uint8_t *code;
    int *lines;
    ValueArray constants;
} Chunk;

typedef struct Obj Obj;

struct Obj {
    ObjType type;
    bool isMarked;
    Obj *next;
};

typedef struct {
    Obj obj;
    int len;
    char *str;
    uint32_t hash;
} ObjString;

typedef struct {
    ObjString *key;
    Value value;
    bool readOnly;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry *entries;
} Table;

typedef struct {
    Obj obj;
    ObjString *name;
    ObjString *path;
    Table values;
    bool used;
} ObjScript;

typedef struct {
    Obj obj;
    int arity;
    int arityDefault;
    int upvalueCount;
    Chunk chunk;
    ObjString *name;
    FunctionType type;
    AccessLevel accessLevel;
    ObjScript *script;
} ObjFunction;

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

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

typedef struct ObjClass {
    Obj obj;
    ObjString *name;
    struct ObjClass *superClass;
    Table methods;
    Table privateMethods;
    Table abstractMethods;
    Table staticVars;
    Table staticConsts;
    Table fields;
    Table privateFields;
    ClassType type;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass *objClass;
    Table fields;
    Table privateFields;
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

typedef struct {
    Obj obj;
    FILE *file;
    char *path;
    char *flags;
} ObjFile;

typedef struct {
    Value key;
    Value value;
    uint32_t psl;
} MapItem;

typedef struct {
    Obj obj;
    int count;
    int capacity;
    MapItem *items;
} ObjMap;

typedef struct {
    Value value;
    bool deleted;
} SetItem;

typedef struct {
    Obj obj;
    int count;
    int capacity;
    SetItem *items;
} ObjSet;

typedef struct ObjAbstract ObjAbstract;
typedef void (*AbstractFreeFn)(VM *vm, ObjAbstract *abstract);

struct ObjAbstract {
    Obj obj;
    Table values;
    void *data;
    AbstractFreeFn feeFn;
};

typedef struct {
    Obj obj;
    struct tm time;
} ObjDateTime;

typedef struct {
    Obj obj;
    GLFWwindow *window;
} ObjWindow;

#ifdef __cplusplus
extern "C" {
#endif

VM *initVM(const char *path, int argc, char **argv);
void freeVM(VM *vm);
void runFile(VM *vm, const char *path);

void runtimeError(VM *vm, const char *format, ...);

void setRuntimeErrorCallback(VM *vm, ErrorCallback runtimeCallback);
void setAssertErrorCallback(VM *vm, ErrorCallback assertCallback);
void setPanicErrorCallback(VM *vm, ErrorCallback panicCallback);

ObjScript *newScript(VM *vm, ObjString* name);
ObjString *copyString(VM *vm, const char* chars, int length);
char *valueType(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

static inline ObjType getObjType(Value value) {
    return AS_OBJ(value)->type;
}

void registerGlobalFunction(VM *vm, const char *name, NativeFn function);
void registerGlobalValue(VM *vm, const char *name, Value value);

void registerLibraryFunction(VM *vm, const char *name, NativeFn function, Table *table);
void registerLibrary(VM *vm, const char *name, BuiltInLib lib);

void registerBaseClass(VM *vm, const char *name);
void registerClassFunction(VM *vm, ObjClass *objClass, const char *name, NativeFn function, AccessLevel accessLevel);
void registerClassVariable(VM *vm, ObjClass *objClass, const char *name, NativeFn function, bool isPrivate);
void registerClassStaticVariable(VM *vm, ObjClass *objClass, const char *name, NativeFn function, bool isConst);


//void registerType(VM *vm, const char *name); //Struct and conversion function
//void registerTypeFunction(VM *vm, const char *type, const char *name, NativeFn function);
//void registerTypeValue(VM *vm, const char *type, const char *name, Value value);
//TODO: Require function to free the type?

#ifdef __cplusplus
}
#endif

#endif //C_ILEX_H
