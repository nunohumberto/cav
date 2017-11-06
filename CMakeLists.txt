cmake_minimum_required(VERSION 3.8)
project(assignment2)



set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        assignment2/wavCopy.cpp)


include_directories(/usr/local/include)

link_directories(/usr/local/lib)


add_executable(assignment2 ${SOURCE_FILES})

target_link_libraries(assignment2 sndfile opencv_core opencv_imgproc opencv_highgui)

