cmake_minimum_required(VERSION 3.12)
project(RayTracing)

# 为什么要3.12，因为这个target风格的cmake就是至少要求3.12版本，特性才能基本支持

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)

find_package(Threads REQUIRED)

add_executable(RayTracing main.cpp Object.hpp Vector.cpp Vector.hpp Sphere.hpp global.hpp Triangle.hpp Scene.cpp
        Scene.hpp Light.hpp AreaLight.hpp BVH.cpp BVH.hpp Bounds3.hpp Ray.hpp Material.hpp Intersection.hpp
        Renderer.cpp Renderer.hpp)

target_compile_features(RayTracing PRIVATE cxx_std_17)
target_link_libraries(RayTracing PRIVATE Threads::Threads)
