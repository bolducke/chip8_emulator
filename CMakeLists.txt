cmake_minimum_required(VERSION 3.11)

message(${CMAKE_CURRENT_SOURCE_DIR})

project(CHIP8 VERSION 1.0.0 DESCRIPTION "" LANGUAGES C CXX)
set(PROJECT_NAME CHIP8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}>)

# ============================================================================
# PACKAGE INSTALLATION
# ============================================================================

## GLAD
#########
set(GLAD_PATH ${CMAKE_SOURCE_DIR}/extern/glad)
add_library(glad STATIC
    ${GLAD_PATH}/src/glad.c
    ${GLAD_PATH}/include/glad/glad.h
)
target_include_directories(glad
    PUBLIC
    ${GLAD_PATH}/include/
)

## SDL2
##########
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# ## IMGUI
# ##########
set(IMGUI_PATH ${CMAKE_SOURCE_DIR}/extern/imgui)

# im-core
add_library(im-core
    ${IMGUI_PATH}/imgui.cpp
    ${IMGUI_PATH}/imgui_demo.cpp    
    ${IMGUI_PATH}/imgui_draw.cpp
    ${IMGUI_PATH}/imgui_widgets.cpp
    ${IMGUI_PATH}/imgui_tables.cpp
    ${IMGUI_PATH}/imgui.h
    ${IMGUI_PATH}/imgui_internal.h
)
target_include_directories(im-core
    PUBLIC
    $<BUILD_INTERFACE:${IMGUI_PATH}>
    $<INSTALL_INTERFACE:imgui.h>
    $<INSTALL_INTERFACE:imgui_internal.h>
)

# im-sdl2
add_library(im-sdl2
  ${IMGUI_PATH}/backends/imgui_impl_sdl2.cpp
  ${IMGUI_PATH}/backends/imgui_impl_sdl2.h
)
target_include_directories(im-sdl2
    PUBLIC
    $<BUILD_INTERFACE:${IMGUI_PATH}/backends>
    $<INSTALL_INTERFACE:imgui_impl_sdl2.h>
)
target_link_libraries(im-sdl2
    im-core
    SDL2::SDL2
  )

# im-opengl
add_library(im-opengl3
  ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
  ${IMGUI_PATH}/backends/imgui_impl_opengl3.h
)
target_include_directories(im-opengl3
    PUBLIC
    $<BUILD_INTERFACE:${IMGUI_PATH}/backends>
    $<INSTALL_INTERFACE:imgui_impl_opengl3.h>
)
target_link_libraries(im-opengl3
    glad
    im-core
)

# ============================================================================
# SRC
# ============================================================================
add_executable(${PROJECT_NAME} src/main.cpp)
target_sources(${PROJECT_NAME}
                    PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/src/emulator.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/src/emulator.h
                        ${CMAKE_CURRENT_LIST_DIR}/src/app.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/src/app.h
)

target_include_directories(${PROJECT_NAME}
                            PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

target_link_libraries(${PROJECT_NAME}
        glad
        im-core
        im-sdl2
        im-opengl3
        SDL2::SDL2
        )