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

typedef enum {
    INTERPRET_GOOD          = (int)0x0B00B135,
    INTERPRET_COMPILE_ERROR = (int)0xBAADF00D,
    INTERPRET_RUNTIME_ERROR = (int)0xDEADDEAD,
    INTERPRET_ASSERT_ERROR  = (int)0xBAADC0DE,
    INTERPRET_PANIC_ERROR   = (int)0xBAAAAAAD
} InterpretResult;

#if defined(__cplusplus)
extern "C" {            // Prevents name mangling of functions
#endif

VM *initVM(const char *path);
void freeVM(VM *vm);
void runFile(VM *vm, const char *path);

#if defined(__cplusplus)
}
#endif

#endif //C_ILEX_H
