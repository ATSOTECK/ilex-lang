cmake_minimum_required(VERSION 3.17)
project(c C)

set(CMAKE_C_STANDARD 99)

add_executable(ilex main.c common.h chunk.h chunk.c memory.h memory.c debug.h debug.c value.h value.c vm.h vm.c compiler.h compiler.c lexer.h lexer.c object.h object.c table.h table.c libs/lib_natives.h libs/lib_natives.c)