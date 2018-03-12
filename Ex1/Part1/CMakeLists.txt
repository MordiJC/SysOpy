cmake_minimum_required(VERSION 3.8.11 FATAL_ERROR)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

add_executable(main main.c)

add_library(blockarraystatic STATIC block_array.c)
target_include_directories(blockarraystatic PUBLIC block_array.h)

add_library(blockarray SHARED block_array.c)
target_include_directories(blockarraystatic PUBLIC block_array.h)

target_link_libraries(main blockarraystatic)

if ( CMAKE_COMPILER_IS_GNUCC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_C_FLAGS} -pedantic -Wall 
    -Wno-missing-braces -Wextra -Wno-missing-field-initializers -Wformat=2 
    -Wswitch-default -Wswitch-enum -Wcast-align -Wpointer-arith 
    -Wbad-function-cast -Wstrict-overflow=5 -Wstrict-prototypes -Winline 
    -Wundef -Wnested-externs -Wcast-qual -Wshadow -Wunreachable-code 
    -Wlogical-op -Wfloat-equal -Wstrict-aliasing=2 -Wredundant-decls 
    -Wold-style-definition -Werror 
    -fno-omit-frame-pointer -ffloat-store -fno-common -fstrict-aliasing")
endif()