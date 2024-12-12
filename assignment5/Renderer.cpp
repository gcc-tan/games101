#include <fstream>
#include "Vector.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include <optional>

inline float deg2rad(const float &deg)
{ return deg * M_PI/180.0; }

// Compute reflection direction
// 参考这篇文章，或者我的作业五问题分析 https://zhuanlan.zhihu.com/p/91129191
Vector3f reflect(const Vector3f &I, const Vector3f &N)
{
    return I - 2 * dotProduct(I, N) * N;
}

// [comment]
// Compute refraction direction using Snell's law
//
// We need to handle with care the two possible situations:
//
//    - When the ray is inside the object
//
//    - When the ray is outside.
//
// If the ray is outside, you need to make cosi positive cosi = -N.I
//
// If the ray is inside, you need to invert the refractive indices and negate the normal N
// [/comment]
Vector3f refract(const Vector3f &I, const Vector3f &N, const float &ior)
{
    float cosi = clamp(-1, 1, dotProduct(I, N));
    float etai = 1, etat = ior;    // etai = 1,真空中的折射率
    Vector3f n = N;
    // 法线方向是物体表面，但是光线可能从类似玻璃球之类的物体里面穿出，因此这里利用i与n的点乘正负来判断
    // 点乘如果是正的，就是上面说的场景
    if (cosi < 0) { cosi = -cosi; } else { std::swap(etai, etat); n= -N; }
    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? 0 : eta * I + (eta * cosi - sqrtf(k)) * n;
}

// [comment]
// Compute Fresnel equation
//
// \param I is the incident view direction
//
// \param N is the normal at the intersection point
//
// \param ior is the material refractive index
// [/comment]
float fresnel(const Vector3f &I, const Vector3f &N, const float &ior)
{
    float cosi = clamp(-1, 1, dotProduct(I, N));
    float etai = 1, etat = ior;
    // 修改了一下原来的代码和refract保持一致
    if (cosi < 0) { cosi = - cosi; } else {  std::swap(etai, etat); }
    // Compute sini using Snell's law
    float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
    // Total internal reflection
    // 根据斯涅尔定律和全反射发生的条件就能想明白了
    // 没用refract里面的k应该是算了没必要，而sint的计算恰好能用来计算cost
    // 这个地方还是写的不错的
    if (sint >= 1) {    
        return 1;
    }
    else {
        float cost = sqrtf(std::max(0.f, 1 - sint * sint));
        // 原来的框架代码菲涅尔方程的rs和rp写反了，不过不影响结果，我修正一下
        float Rp = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rs = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        return (Rs * Rs + Rp * Rp) / 2;
    }
    // As a consequence of the conservation of energy, transmittance is given by:
    // kt = 1 - kr;
}

// [comment]
// Returns true if the ray intersects an object, false otherwise.
//
// \param orig is the ray origin
// \param dir is the ray direction
// \param objects is the list of objects the scene contains
// \param[out] tNear contains the distance to the cloesest intersected object.
// \param[out] index stores the index of the intersect triangle if the interesected object is a mesh.
// \param[out] uv stores the u and v barycentric coordinates of the intersected point
// \param[out] *hitObject stores the pointer to the intersected object (used to retrieve material information, etc.)
// \param isShadowRay is it a shadow ray. We can return from the function sooner as soon as we have found a hit.
// [/comment]
std::optional<hit_payload> trace(
        const Vector3f &orig, const Vector3f &dir,
        const std::vector<std::unique_ptr<Object> > &objects)
{
    float tNear = kInfinity;    // 当前光线与所有相交物体的最小值
    std::optional<hit_payload> payload;
    for (const auto & object : objects)
    {
        float tNearK = kInfinity;    // 相交物体的当前值
        uint32_t indexK;
        Vector2f uvK;
        if (object->intersect(orig, dir, tNearK, indexK, uvK) && tNearK < tNear)
        {
            payload.emplace();
            payload->hit_obj = object.get();
            payload->tNear = tNearK;
            payload->index = indexK;
            payload->uv = uvK;
            tNear = tNearK;
        }
    }

    return payload;
}

// [comment]
// Implementation of the Whitted-style light transport algorithm (E [S*] (D|G) L)
//
// This function is the function that compute the color at the intersection point
// of a ray defined by a position and a direction. Note that thus function is recursive (it calls itself).
//
// If the material of the intersected object is either reflective or reflective and refractive,
// then we compute the reflection/refraction direction and cast two new rays into the scene
// by calling the castRay() function recursively. When the surface is transparent, we mix
// the reflection and refraction color using the result of the fresnel equations (it computes
// the amount of reflection and refraction depending on the surface normal, incident view direction
// and surface refractive index).
//
// If the surface is diffuse/glossy we use the Phong illumation model to compute the color
// at the intersection point.
// [/comment]
Vector3f castRay(
        const Vector3f &orig, const Vector3f &dir, const Scene& scene,
        int depth)
{
    if (depth > scene.maxDepth) {
        return Vector3f(0.0,0.0,0.0);
    }

    Vector3f hitColor = scene.backgroundColor;
    if (auto payload = trace(orig, dir, scene.get_objects()); payload)
    {
        Vector3f hitPoint = orig + dir * payload->tNear;
        Vector3f N; // normal
        Vector2f st; // st coordinates
        payload->hit_obj->getSurfaceProperties(hitPoint, dir, payload->index, payload->uv, N, st);
        switch (payload->hit_obj->materialType) {
            case REFLECTION_AND_REFRACTION:
            {
                Vector3f reflectionDirection = normalize(reflect(dir, N));
                Vector3f refractionDirection = normalize(refract(dir, N, payload->hit_obj->ior));
                Vector3f reflectionRayOrig = (dotProduct(reflectionDirection, N) < 0) ?
                                             hitPoint - N * scene.epsilon :
                                             hitPoint + N * scene.epsilon;
                Vector3f refractionRayOrig = (dotProduct(refractionDirection, N) < 0) ?
                                             hitPoint - N * scene.epsilon :
                                             hitPoint + N * scene.epsilon;
                Vector3f reflectionColor = castRay(reflectionRayOrig, reflectionDirection, scene, depth + 1);
                Vector3f refractionColor = castRay(refractionRayOrig, refractionDirection, scene, depth + 1);
                // 在又有反射又有折射的情况hit_obj的颜色是两者的和，比例就是用菲涅尔方程算出来的
                float kr = fresnel(dir, N, payload->hit_obj->ior);
                hitColor = reflectionColor * kr + refractionColor * (1 - kr);
                break;
            }
            case REFLECTION:
            {
                float kr = fresnel(dir, N, payload->hit_obj->ior);
                // 公式算出来的就是单位向量，不做标准化也没问题，问题是可能有精度误差，还有就是前面的做后面的，这显得比较诡异
                Vector3f reflectionDirection = reflect(dir, N);    
                // 下面这个epsilon的加减也调整了一下，应该是有问题的，这样调整之后和REFLECTION_AND_REFRACTIONd的也保持一致
                // 加减是因为浮点运算有精度误差
                // 反射的加减是为了保证反射后光线的起点和入射光线在相同的介质中
                // 折射就是为了保证在不同的介质中
                // 从而保证光线计算的正确性
                Vector3f reflectionRayOrig = (dotProduct(reflectionDirection, N) < 0) ?
                                             hitPoint - N * scene.epsilon :
                                             hitPoint + N * scene.epsilon;
                hitColor = castRay(reflectionRayOrig, reflectionDirection, scene, depth + 1) * kr;
                break;
            }
            default:
            {
                // [comment]
                // We use the Phong illumation model int the default case. The phong model
                // is composed of a diffuse and a specular reflection component.
                // [/comment]
                Vector3f lightAmt = 0, specularColor = 0;
                // 这个判断还是有点意思，dotProduct(dir, N)，仔细想想就知道目前的代码，在这个default判断中不可能有点乘 > 0的情况
                // 因为感知光线dir和表面法线N点乘大于零表示从物体内部射出，那既然要从内部射出，那感知光线进入的时候物体类型必然是REFLECTION_AND_REFRACTION
                // 因此是那就进入不了这个Phong光照模型的计算，需要再进行castRay的递归
                // 额除非这个物体，一部分是REFLECTION_AND_REFRACTION，一部分是DIFFUSE_AND_GLOSSY，玻璃球套一半塑料？
                Vector3f shadowPointOrig = (dotProduct(dir, N) < 0) ?
                                           hitPoint + N * scene.epsilon :
                                           hitPoint - N * scene.epsilon;
                // [comment]
                // Loop over all lights in the scene and sum their contribution up
                // We also apply the lambert cosine law
                // [/comment]
                for (auto& light : scene.get_lights()) {
                    Vector3f lightDir = light->position - hitPoint;
                    // square of the distance between hitPoint and the light
                    float lightDistance2 = dotProduct(lightDir, lightDir);
                    lightDir = normalize(lightDir);
                    float LdotN = std::max(0.f, dotProduct(lightDir, N));
                    // is the point in shadow, and is the nearest occluding object closer to the object than the light itself?
                    auto shadow_res = trace(shadowPointOrig, lightDir, scene.get_objects());
                    // 感觉inShadow的判断shadow_res.has_value，从hitPoint到光源的的shadow ray有物体遮挡就可以了? 这个时间t和距离判断也是要理解一会
                    // 不过也没毛病，o + td，t就相当于距离，因为d是单位向量
                    bool inShadow = shadow_res && (shadow_res->tNear * shadow_res->tNear < lightDistance2);

                    // 使用的是Phong光照模型，而不是Blinn-Phong   https://blog.wallenwang.com/2017/03/phong-lighting-model/
                    // lightAmt这个名字有点诡异，明明属于漫反射项，而且没有考虑hitPoint和光源的距离？
                    lightAmt += inShadow ? 0 : light->intensity * LdotN;
                    Vector3f reflectionDirection = reflect(-lightDir, N);

                    // 高光不用判断遮挡？
                    specularColor += powf(std::max(0.f, -dotProduct(reflectionDirection, dir)),
                        payload->hit_obj->specularExponent) * light->intensity;
                }

                hitColor = lightAmt * payload->hit_obj->evalDiffuseColor(st) * payload->hit_obj->Kd + specularColor * payload->hit_obj->Ks;
                break;
            }
        }
    }

    return hitColor;
}

// [comment]
// The main render function. This where we iterate over all pixels in the image, generate
// primary rays and cast these rays into the scene. The content of the framebuffer is
// saved to a file.
// [/comment]
void Renderer::Render(const Scene& scene)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    float scale = std::tan(deg2rad(scene.fov * 0.5f));
    float imageAspectRatio = scene.width / (float)scene.height;    // 成像平面的宽高比
    float zNear = 1;    // 近平面距离，也就是成像平面
    float w = imageAspectRatio * 2 * zNear * scale;    // c成像平面的实际大小宽高，计算可以参考我的作业五的问题与疑惑
    float h = 2 * zNear * scale;

    // Use this variable as the eye position to start your rays.
    Vector3f eye_pos(0);
    int m = 0;
    for (int j = 0; j < scene.height; ++j)
    {
        for (int i = 0; i < scene.width; ++i)
        {
            // generate primary ray direction
            float x = ((float)i) / scene.width * w - w / 2;
            float y = (-(float)j) / scene.height * h + h / 2;
            // Find the x and y positions of the current pixel to get the direction
            // vector that passes through it.
            // Also, don't forget to multiply both of them with the variable *scale*, and
            // x (horizontal) variable with the *imageAspectRatio*            

            // Vector3f dir = Vector3f(x, y, -1); // Don't forget to normalize this direction!
            // 原来的这个-1的代码，有点不是那么直观，想了一会才看明白
            // 这个dir向量是相机连向成像平面上像素点的，因此这个向量的z坐标就是zNear，加上符号即可，因为向z负半方向看
            Vector3f dir = Vector3f(x, y, -zNear); 
            dir = normalize(dir);
            framebuffer[m++] = castRay(eye_pos, dir, scene, 0);
        }
        UpdateProgress((j + 1) / (float)scene.height);    // 这个代码不改我难受
    }

    // save framebuffer to file
    // 还在好奇cmake里面居然没有用opencv，那结果怎么展示
    // 原来是用的ppm文件，之前没见过，网上搜了一下，文件格式比较简答
    /*
        文件头由3行文本组成，可由fgets读出
        1）第一行为“P6"，表示文件类型
        2）第二行为图像的宽度和高度
        3）第三行为最大的象素值
        接下来是图像数据块。按行顺序存储。每个象素占3个字节，依次为红绿蓝通道，每个通道为1字节整
        数。左上角为坐标原点。
    */
    // https://blog.csdn.net/zhanshen112/article/details/124023982
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (char)(255 * clamp(0, 1, framebuffer[i].x));
        color[1] = (char)(255 * clamp(0, 1, framebuffer[i].y));
        color[2] = (char)(255 * clamp(0, 1, framebuffer[i].z));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);    
}
