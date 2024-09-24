cmake_minimum_required(VERSION 3.10)
project(Rasterizer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED CONFIG)


add_executable(Rasterizer main.cpp rasterizer.hpp rasterizer.cpp global.hpp Triangle.hpp Triangle.cpp)
target_link_libraries(Rasterizer 
PRIVATE 
    opencv_core opencv_imgcodecs opencv_imgproc opencv_highgui
    Eigen3::Eigen
)