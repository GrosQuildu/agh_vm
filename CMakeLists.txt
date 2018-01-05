cmake_minimum_required (VERSION 2.6)
project (VM)
option(BUILD_ARM "Build for ARM architecture" ON)

set (PROJECT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
file(GLOB VM_cpp "${PROJECT_SOURCE_DIR}/*.cpp")
set (VM_SOURCES "${PROJECT_SOURCE_DIR}/Exceptions.h" "${VM_cpp}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}")

if(BUILD_ARM)
    set (CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabi-gcc)
    set (CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabi-g++)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DDEBUG=0")
else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -lncurses -DDEBUG=1")
endif()

add_executable(VM ${VM_SOURCES})