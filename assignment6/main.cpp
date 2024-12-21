#include "Renderer.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "Vector.hpp"
#include "global.hpp"
#include <chrono>

// In the main function of the program, we create the scene (create objects and
// lights) as well as set the options for the render (image width and height,
// maximum recursion depth, field-of-view, etc.). We then call the render
// function().
int main(int argc, char** argv)
{
    Scene scene(1280, 960);

    MeshTriangle bunny("../models/bunny/bunny.obj");

    /*
     * 为助教的框架代码点个赞，助教对类的设计和逻辑的抽象是值得学习的
     * 特别是MeshTriangle和Bounds类的设计
     * 首先Object类是父类，表示场景中的一切物体，简单的包括Triangle，Sphere。
     * 然后复杂的包括MeshTriangle，MeshTriangle是Triangle组成的复杂物体，这些类都是Object的子类
     * 优点包括：
     * 1. 这样设计保证了场景中添加物体，然后管理物体的便利性，以及场景渲染逻辑的独立性，保证了代码的可读性
     * 2. 可以仔细看这个BVH加速类的设计，场景对于场景中的物体做了一次BVH，然后对于低一层级的复杂物体，像MeshTriangle，也有自己的BVH，
     *    这种分层结构很好地实现了BVH加速的逻辑：光线和场景物体的Bounding Box没有交集，自然和场景物体没有交集。如果有交集再次利用场景物体自己的BVH加速
     * p.s 放心不是彩虹屁，我也不交作业
     */
    scene.Add(&bunny);
    scene.Add(std::make_unique<Light>(Vector3f(-20, 70, 20), 1));
    scene.Add(std::make_unique<Light>(Vector3f(20, 70, 20), 1));
    scene.buildBVH();

    Renderer r;

    auto start = std::chrono::system_clock::now();
    r.Render(scene);
    auto stop = std::chrono::system_clock::now();

    std::cout << "Render complete: \n";
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::hours>(stop - start).count() << " hours\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::minutes>(stop - start).count() << " minutes\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() << " seconds\n";

    return 0;
}