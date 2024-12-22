//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include "Scene.hpp"
#include "Renderer.hpp"


inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    /*
     * 如果相机在原点，其实好理解，按照之前我作业五里面的说法，向世界坐标的-z看，相机的正方向是世界坐标+y
     * 那对于相机不在原点的情况呢？只能按照原来的假设，还是向-z看，正方向是+y。那对于这种情况，其实变化的就是zNear，即相机与成像平面的距离
     * 经过作业五的计算，光线向量与zNear并没有关系，因此就可以把相机不在原点，当成相机在原点的情况计算光线向量
    */
    Vector3f eye_pos(-1, 5, 10);
    int m = 0;
    for (int j = 0; j < scene.height; ++j) {
        for (int i = 0; i < scene.width; ++i) {
            // generate primary ray direction
            float x = (2 * (i + 0.5) / (float)scene.width - 1) *
                      imageAspectRatio * scale;
            float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;
            //  Find the x and y positions of the current pixel to get the
            // direction
            //  vector that passes through it.
            // Also, don't forget to multiply both of them with the variable
            // *scale*, and x (horizontal) variable with the *imageAspectRatio*
            // 就不用作业五的包含zNear和w，h的表达式了
            Vector3f dir = Vector3f(x, y, -1);
            dir = normalize(dir);
            // Don't forget to normalize this direction!
            framebuffer[m++] = scene.castRay(Ray(eye_pos, dir), 0);    // 居然没有更新framebuffer，帧缓存，难怪看不到
        }
        UpdateProgress(j / (float)scene.height);
    }
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * clamp(0, 1, framebuffer[i].x));
        color[1] = (unsigned char)(255 * clamp(0, 1, framebuffer[i].y));
        color[2] = (unsigned char)(255 * clamp(0, 1, framebuffer[i].z));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);    
}
