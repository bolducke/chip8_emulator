cmake_minimum_required(VERSION 3.24.2)

project(Glad)

set(GLAD_PATH ${CMAKE_CURRENT_SOURCE_DIR})
add_library(glad STATIC
    ${GLAD_PATH}/src/glad.c
    ${GLAD_PATH}/include/glad/glad.h
)
set_target_properties(glad PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(glad
    PUBLIC
    ${GLAD_PATH}/include/
)