cmake_minimum_required(VERSION 3.10)
project(Rasterizer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)



add_executable(Rasterizer main.cpp rasterizer.hpp rasterizer.cpp Triangle.hpp Triangle.cpp)
target_link_libraries(Rasterizer PRIVATE opencv_core opencv_imgcodecs opencv_highgui)
