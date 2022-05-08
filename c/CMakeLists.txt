cmake_minimum_required(VERSION 3.17)
project(c C)

set(CMAKE_C_STANDARD 99)

add_executable(ilex main.c common.h chunk.h chunk.c memory.h memory.c debug.h debug.c value.h value.c vm.h vm.c compiler.h compiler.c lexer.h lexer.c object.h object.c table.h table.c libs/lib_natives.h libs/lib_natives.c libs/lib_string.h libs/lib_string.c ilex_include.h libs/lib_builtIn.h libs/lib_builtIn.c libs/lib_math.h libs/lib_math.c libs/lib_ilex.h libs/lib_ilex.c libs/lib_io.h libs/lib_io.c libs/lib_random.h libs/lib_random.c libs/lib_array.h libs/lib_array.c)