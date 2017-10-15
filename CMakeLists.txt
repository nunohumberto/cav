cmake_minimum_required(VERSION 3.8)
project(cav)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        #assignment1/main.cpp
        lab1/ex1.cpp
        #lab1/ex2.cpp
        )

add_executable(cav ${SOURCE_FILES})