//
// Created by Skyler on 11/12/21.
//

#ifndef C_COMMON_H
#define C_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//#define DEBUG_PRINT_CODE
//#define DEBUG_TRACE_EXECUTION

//#define DEBUG_STRESS_GC
//#define DEBUG_LOG_GC

#define UINT8_COUNT (UINT8_MAX + 1)

#define ILEX_VERSION_MAJOR 0
#define ILEX_VERSION_MINOR 0
#define ILEX_VERSION_BUILD 14
#define ILEX_VERSION "0.0.14"

#if defined(WIN32) || defined(_WIN32)
#   define I_WIN
#   include <Windows.h>
#endif

#if defined(__APPLE__)
#   define I_MAC
#endif

#endif //C_COMMON_H
