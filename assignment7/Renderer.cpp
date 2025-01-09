//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include "Scene.hpp"
#include "Renderer.hpp"
#include <functional>


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
    Vector3f eye_pos(278, 273, -800);
    int m = 0;

    // change the spp value to change sample ammount
    // 其实如果线程池对spp进行划分，而不是像现在这样对行操作（每个线程执行row_oper）代码改动会非常少而且也好理解
    // 但是带来的问题就是线程池的每个线程只执行一次castRay，这样计算时间非常短，主要的消耗就在，线程池的线程归还和切换，还有线程启动上
    // 可以用top -H -p pid查看运行状态，发现线程的CPU负载在40%左右
    // 而改用这种划分单个线程可以跑满CPU
    int spp = 32;
    std::cout << "SPP: " << spp << "\n";
    std::vector< std::future<std::vector<Vector3f>>> results(scene.height);
    auto row_oper = [imageAspectRatio, &scene ,scale, spp, &eye_pos](int param_j) -> std::vector<Vector3f> {
        std::vector<Vector3f> row_frame(scene.width);
        for (uint32_t i = 0; i < scene.width; ++i) {
            // generate primary ray direction
            float x = (2 * (i + 0.5) / (float)scene.width - 1) *
                      imageAspectRatio * scale;
            float y = (1 - 2 * (param_j + 0.5) / (float)scene.height) * scale;

            Vector3f dir = normalize(Vector3f(-x, y, 1));
            for (int k = 0; k < spp; k++){
                row_frame[i] += scene.castRay(Ray(eye_pos, dir), 0) / spp;  
            }
        }
        return row_frame;
    };
    for (uint32_t j = 0; j < scene.height; ++j) {
        results[j] = executor.commit(row_oper, j);
    }
    for (uint32_t j = 0; j < scene.height; ++j) {
        std::vector<Vector3f> row_frame = results[j].get();
        for (uint32_t i = 0; i < scene.width; ++i) 
        {
            framebuffer[j * scene.width + i] = row_frame[i];
        }
        UpdateProgress(j / (float)scene.height);
    }
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);    
}
