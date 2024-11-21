cmake_minimum_required(VERSION 3.10)
project(BezierCurve)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(OpenCV REQUIRED)



add_executable(BezierCurve main.cpp)
target_link_libraries(BezierCurve 
PRIVATE 
    opencv_core opencv_imgcodecs opencv_imgproc opencv_highgui
)
