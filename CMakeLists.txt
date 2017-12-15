cmake_minimum_required (VERSION 2.6)
project (VM)
add_executable(VM VM.cpp VM.h Function.cpp Function.h Thread.cpp Thread.h ThreadManager.cpp ThreadManager.h main.cpp Exceptions.cpp Exceptions.h Helpers.cpp Helpers.h)