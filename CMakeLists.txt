cmake_minimum_required(VERSION 3.8)
project(cav)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        #cav/assignment1/main.cpp
        cav/lab1/ex1.cpp
        #cav/lab1/ex2.cpp
        )

add_executable(cav ${SOURCE_FILES})