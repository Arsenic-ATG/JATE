cmake_minimum_required(VERSION 3.0)

project(JATE)


file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*.c")
add_executable(JATE ${SOURCES})

