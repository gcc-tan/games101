cmake_minimum_required(VERSION 3.10)
project(homework0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Eigen3 REQUIRED CONFIG)


add_executable(main main.cpp)
target_link_libraries(main PRIVATE Eigen3::Eigen)