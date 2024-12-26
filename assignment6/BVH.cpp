#include <algorithm>
#include <cassert>
#include "BVH.hpp"
#include <chrono>

BVHAccel::BVHAccel(std::vector<Object*> p, int nodeMaxPrims,
                   SplitMethod sp)
    : maxPrimsInNode(std::min(255, nodeMaxPrims)), splitMethod(sp),
      primitives(std::move(p))
{
    time_t start, stop;
    time(&start);
    if (primitives.empty())
        return;

    root = recursiveBuild(primitives);

    time(&stop);
    double diff = difftime(stop, start);
    int hrs = (int)diff / 3600;
    int mins = ((int)diff / 60) - (hrs * 60);
    int secs = (int)diff - (hrs * 3600) - (mins * 60);

    printf(
        "\rBVH Generation complete: \nTime Taken: %i hrs, %i mins, %i secs\n\n",
        hrs, mins, secs);
}

BVHBuildNode* BVHAccel::recursiveBuild(std::vector<Object*> objects)
{
    BVHBuildNode* node = new BVHBuildNode();

    // Compute bounds of all primitives in BVH node
    Bounds3 bounds;
    for (int i = 0; i < (int)objects.size(); ++i)
        bounds = Union(bounds, objects[i]->getBounds());

    auto makeLeaf = [&objects](BVHBuildNode *target_node) -> void {
        // Create leaf _BVHBuildNode_
        Bounds3 node_bounds;
        for (auto *obj : objects) {
        node_bounds = Union(obj->getBounds(), node_bounds);
        }
        target_node->bounds = node_bounds;
        target_node->object = objects;
        target_node->left = nullptr;
        target_node->right = nullptr;
    };
    if ((int)objects.size() <= maxPrimsInNode) 
    {
        makeLeaf(node);
        return node;
    }

    Bounds3 centroidBounds;
    // 利用物体包围盒的几何中心坐标（Centroid）计算这些中心点的包围盒
    for (int i = 0; i < (int)objects.size(); ++i)
        centroidBounds = Union(centroidBounds, objects[i]->getBounds().Centroid());
    // 利用上面计算包围盒的对角线，然后分别投影到x，y，z上，看哪个分量最长，就按照哪个分量划分
    int dim = centroidBounds.maxExtent();

    std::vector<Object*> leftshapes;
    std::vector<Object*> rightshapes;
    switch(splitMethod) 
    {
        case SplitMethod::NAIVE:
        {
            switch (dim) {
            case 0:
                std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                    return f1->getBounds().Centroid().x <
                            f2->getBounds().Centroid().x;
                });
                break;
            case 1:
                std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                    return f1->getBounds().Centroid().y <
                            f2->getBounds().Centroid().y;
                });
                break;
            case 2:
                std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                    return f1->getBounds().Centroid().z <
                            f2->getBounds().Centroid().z;
                });
                break;
            }
            auto beginning = objects.begin();
            auto middling = objects.begin() + (objects.size() / 2);
            auto ending = objects.end();

            leftshapes = std::vector<Object*>(beginning, middling);
            rightshapes = std::vector<Object*>(middling, ending);

            break;
        }
        case SplitMethod::SAH:
        {
            struct BucketData
            {
                int count;
                Bounds3 bound;
                BucketData() : count(0) {}
            };
            std::vector<BucketData> bucket(bucketSize);

            // 计算obj所属的bucket索引，其实cmu的ppt拿的是图元的包围盒去切的桶，而不是图元的质心，不过我感觉区别不大
            auto getBucketIndex = [dim, &centroidBounds](Object *obj) -> int {
              const Vector3f offset = centroidBounds.Offset(obj->getBounds().Centroid());
              int bucket_idx = offset[dim] * bucketSize;
              if (bucket_idx < 0)
                bucket_idx = 0;
              if (bucket_idx >= bucketSize)
                bucket_idx = bucketSize - 1;
              return bucket_idx;
            };
            // 遍历当前的图元（物体），找到每个图元centroid所属的桶，然后更新桶的范围和数量
            for (auto* obj : objects)
            {
                int bucket_idx = getBucketIndex(obj);

                ++bucket[bucket_idx].count;
                bucket[bucket_idx].bound = Union(obj->getBounds(), bucket[bucket_idx].bound);
            }

            // 划分只会出现在桶与桶之间，即<0>与<1, 2, ... bucketSize - 1> 是一种
            // <0,1> 和 <2, 3, ... bucketSize - 1>又是一种
            // 遍历这些组合，计算cost最小的一个
            float min_cost = std::numeric_limits<float>::max(), cur_cost;
            Bounds3 left_bounds;
            int left_count = 0;
            int partition_idx = -1;
            for (int i = 0; i < bucketSize; ++i)
            {
                left_count += bucket[i].count;
                left_bounds = Union(bucket[i].bound, left_bounds);

                Bounds3 right_bounds;
                int right_count = 0;
                for (int j = i + 1; j < bucketSize; ++j)
                {
                    right_bounds = Union(bucket[j].bound, right_bounds);
                    right_count += bucket[j].count;
                }
                cur_cost = (left_count * left_bounds.SurfaceArea() +
                            right_count * right_bounds.SurfaceArea()) /
                           Union(left_bounds, right_bounds).SurfaceArea();
                if (cur_cost < min_cost)
                {
                    min_cost = cur_cost;
                    partition_idx = i;
                }
            }

            // 根据桶索引的左右进行划分
            for (auto* obj : objects)
            {
                int bucket_idx = getBucketIndex(obj);
                if (bucket_idx <= partition_idx)
                    leftshapes.push_back(obj);
                else
                    rightshapes.push_back(obj);
            }
            // 划分不出来的情况，见http://15462.courses.cs.cmu.edu/fall2017/lecture/acceleratingqueries/slide_028
            if (leftshapes.empty() || rightshapes.empty())
            {
                makeLeaf(node);
                return node;
            }
            break;
        }
    }

    assert(objects.size() == (leftshapes.size() + rightshapes.size()));

    node->left = recursiveBuild(leftshapes);
    node->right = recursiveBuild(rightshapes);

    node->bounds = Union(node->left->bounds, node->right->bounds);
    return node;
}

Intersection BVHAccel::Intersect(const Ray& ray) const
{
    Intersection isect;
    if (!root)
        return isect;
    isect = BVHAccel::getIntersection(root, ray);
    return isect;
}

Intersection BVHAccel::getIntersection(BVHBuildNode* node, const Ray& ray) const
{
    //  Traverse the BVH to find intersection
    if (node == nullptr)
    {
        return {};
    }
    // 与整个包围盒没有交点
    if (!node->bounds.IntersectP(ray))
            
    {
        return {};
    }

    // 叶子节点
    if (node->left == nullptr && node->right == nullptr)
    {
        // todo这里很奇怪，光线为啥不用引用
        // 好吧为了重新计算direction的direction_inv，说是为了加速（将除法改成乘法），然后引入了对象拷贝的开销
        // 真的能够加速吗....
        Intersection inter_result;
        for (auto* obj : node->object)
        {
            auto obj_inter = obj->getIntersection(ray);
            if (obj_inter.happened && obj_inter.distance < inter_result.distance)
            {
                inter_result = obj_inter;
            }
        }
        return inter_result;
    }

    Intersection inter_left;
    inter_left = getIntersection(node->left, ray);

    Intersection inter_right;
    inter_right = getIntersection(node->right, ray);

    return inter_left.distance < inter_right.distance ? inter_left : inter_right;
}