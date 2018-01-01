cmake_minimum_required (VERSION 2.6)
project (VM)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lncurses -Wall")
add_executable(VM main.cpp VM.cpp Thread.cpp  Exceptions.cpp Helpers.cpp Function.cpp)