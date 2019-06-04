# Created By Palette25 - 2019/06/04

cmake_minimum_required(VERSION 3.10)
project(HDR-Transformer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

FILE(GLOB_RECURSE SOURCE ${SOURCE_DIR}/*.cpp ${SOURCE_DIR}/*.c)
FILE(GLOB_RECURSE HEADER ${SOURCE_DIR}/*.h ${SOURCE_DIR}/*.hpp)

add_subdirectory(dependencies/glad)

FILE(GLOB DEPENDENCIES_SOURCE dependencies/src/*.c dependencies/src/*.cpp)
FILE(GLOB ALL_SOURCES ${SOURCE} ${HEADER} ${DEPENDENCIES_SOURCE})

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

set(EXECUTABLE_NAME ${PROJECT_NAME})

add_executable(${EXECUTABLE_NAME} ${ALL_SOURCES})

# copy resources and shaders after build
add_custom_target(copy-resources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/resources ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources)

target_link_libraries(${EXECUTABLE_NAME} glad)
add_dependencies(${EXECUTABLE_NAME} copy-resources)