//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    /*
     * 一个典型的按照发光区域面积为权重的随机选择算法

     * en 可能游戏抽奖好久没写了，有些生疏了。主要纠结的问题在于要不要对物体顺序做random_shuffle
     * 其实仔细想想没有必要，其实整个过程就是举个例子：
     * 假设有权重数组[1, 2, 5]，那么需要做的第一步就是，将前缀的权重加起来
     * 即[1, 3, 8]，然后生成一个[0, 8)的随机数p，p如果在[0, 1)那就是索引0（即权重1对应的数），p在[1, 2)那就是索引1（即权重2对应的数），p在[2, 8)那就是索引
     * 由于权重数组的非负，因此前缀的权重数组是个有序的，可以使用二分查找判断随机数掉落区间进行加速
     * 但是总体时间复杂度都是O(n)
     */
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TO DO Implement Path Tracing Algorithm here
    auto p = intersect(ray);
    return shade(p, ray.direction);
}


// 作业里有提示，这个wo和课程ppt里面的方向是相反的
// 因此下面代码的wo是说从相机（当前观察方向）到着色点p的方向
Vector3f Scene::shade(const Intersection& p, Vector3f wo) const
{
    if (!p.happened)
    {
        return Vector3f(0);
    }
    // 先对面光源均匀采样，采样点的位置用x表示
    float pdf_light;
    Intersection x;
    sampleLight(x, pdf_light);

    Vector3f ws = x.coords - p.coords;
    float distance = ws.norm();
    ws = normalize(ws);
    // 然后发出一条光线px，即检查着色点p和光源x之间是否有遮挡
    auto inter = intersect(Ray(p.coords, ws));
    // 没有遮挡则计算直接光照
    Vector3f l_dir(0);
    if (inter.happened && std::fabs(inter.distance - distance) < EPSILON)    
    {
        // ws方向，感觉后面一个应该有问题
        l_dir = inter.emit * inter.m->eval(wo, ws, p.normal) * dotProduct(ws, inter.normal) / (distance * distance) / pdf_light;
    }

    // 间接光照
    Vector3f l_indir(0);
    if (get_random_float() < RussianRoulette)
    {
        Vector3f wi = normalize(p.m->sample(wo, p.normal));
        auto q = intersect(Ray(p.coords, wi));
        if (q.happened && !q.m->hasEmission())    // 这个随机采样的wi方向打到了不发光的物体
        {
            l_indir = shade(q, wi) * p.m->eval(wo, wi, p.normal) *
                    dotProduct(wi, p.normal) / p.m->pdf(wo, wi, p.normal) / RussianRoulette;
        }
    }

    return l_dir + l_indir;
}