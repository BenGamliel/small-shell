cmake_minimum_required(VERSION 3.10)
project(HW1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -pedantic-errors")

set(SOURCE_FILES
        Commands.h Commands.cpp
        smash.cpp
        signals.h signals.cpp)
add_executable(HW1 ${SOURCE_FILES})