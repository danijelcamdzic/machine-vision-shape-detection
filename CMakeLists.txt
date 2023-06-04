cmake_minimum_required(VERSION 3.1)
project(shape-detection-app)

set(CMAKE_CXX_STANDARD 14)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(shape-detection-app src/main.cpp)

target_link_libraries(shape-detection-app ${OpenCV_LIBS})