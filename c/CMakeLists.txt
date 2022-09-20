cmake_minimum_required(VERSION 3.17)
project(c C)

set(CMAKE_C_STANDARD 99)

set(sources
        ilex.h
        chunk.h
        chunk.c
        memory.h
        memory.c
        debug.h
        debug.c
        value.h
        value.c
        vm.h
        vm.c
        compiler.h
        compiler.c
        lexer.h
        lexer.c
        object.h
        object.c
        table.h
        table.c
        libs/lib_natives.h
        libs/lib_natives.c
        libs/lib_string.h
        libs/lib_string.c
        libs/lib_builtIn.h
        libs/lib_builtIn.c
        libs/lib_math.h
        libs/lib_math.c
        libs/lib_ilex.h
        libs/lib_ilex.c
        libs/lib_io.h
        libs/lib_io.c
        libs/lib_random.h
        libs/lib_random.c
        libs/lib_array.h
        libs/lib_array.c
        libs/lib_env.h
        libs/lib_env.c
        libs/lib_file.h
        libs/lib_file.c
        util.h
        util.c
        libs/lib_map.h
        libs/lib_map.c
        libs/lib_sys.h
        libs/lib_sys.c
        libs/lib_set.h
        libs/lib_set.c
        libs/json/json.h
        libs/json/json.c
        libs/json/json-builder.h
        libs/json/json-builder.c
        libs/lib_json.h
        libs/lib_json.c
        libs/lib_time.h
        libs/lib_time.c)

add_executable(ilex main.c ${sources})

add_library(ilex_lib ${sources})