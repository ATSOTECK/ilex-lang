cmake_minimum_required(VERSION 3.17)
project(c C)

set(CMAKE_C_STANDARD 99)

include_directories(${PROJECT_SOURCE_DIR}/inc)
link_directories(${PROJECT_SOURCE_DIR}/lib)

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
        types/type_string.h
        types/type_string.c
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
        types/type_array.h
        types/type_array.c
        libs/lib_env.h
        libs/lib_env.c
        types/type_file.h
        types/type_file.c
        util.h
        util.c
        types/type_map.h
        types/type_map.c
        libs/lib_sys.h
        libs/lib_sys.c
        types/type_set.h
        types/type_set.c
        libs/json/json.h
        libs/json/json.c
        libs/json/json-builder.h
        libs/json/json-builder.c
        libs/lib_json.h
        libs/lib_json.c
        libs/lib_time.h
        libs/lib_time.c
        glad.c)

add_executable(ilex main.c ${sources} types/type_window.h types/type_window.c libs/lib_window.h libs/lib_window.c libs/lib_test.h libs/lib_test.c libs/lib_ascii.h libs/lib_ascii.c)

add_library(ilex_lib ${sources})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS} -I/usr/local/lib -framework Cocoa -framework IOKit -framework CoreVideo -framework OpenGL -lglfw3")