cmake_minimum_required(VERSION 3.8.11 FATAL_ERROR)

add_executable(main main.c)

add_library(blockarraystatic STATIC block_array.c)
target_include_directories(blockarraystatic PUBLIC block_array.h)

add_library(blockarray SHARED block_array.c)
target_include_directories(blockarraystatic PUBLIC block_array.h)
