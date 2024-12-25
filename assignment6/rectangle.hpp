#pragma once

#include "Object.hpp"
#include "Bounds3.hpp"
#include "Vector.hpp"

/*
我添加的矩形轴对称矩形物体，支持透明材质，是的就是头像，不过不知道渲染的对不对
添加的目的是想做bvh的可视化，因为框架中的划分缺点就是包围盒重叠的太多导致加速失效
auto m1 =
        new Material(MaterialType::REFLECTION_AND_REFRACTION,
                        Vector3f(0.8, 0, 0), Vector3f(0, 0, 0));
    m1->Kd = 0.6;
    m1->Ks = 0.0;
    m1->ior = 1.5;
    m1->specularExponent = 0;
    auto m2 =
        new Material(MaterialType::DIFFUSE_AND_GLOSSY,
                        Vector3f(0, 0.8, 0), Vector3f(0, 0, 0));
    m2->Kd = 0.6;
    m2->Ks = 0.0;
    m2->specularExponent = 0;
    Bounds3 front_b(Vector3f{0, 0, 0}, Vector3f{3, 3 ,3});
    Bounds3 end_b(Vector3f{0, 0, -4}, {3, 3, -1});
    AARectangle front(front_b, m1);
    AARectangle end(end_b, m2);
    scene.Add(&front);
    scene.Add(&end);
*/

class AARectangle : public Object{
public:
    Bounds3 bound;
    Material *m;
    bool intersect(const Ray&) override { return false; }
    bool intersect(const Ray&, float &, uint32_t &) const override { return false;}

    AARectangle(const Bounds3 &_bound, Material* _m) : bound(_bound), m(_m) {}

    Intersection getIntersection(Ray ray) override{
        Intersection result;
        float enter_time;
        float exit_time;
        result.happened = bound.IntersectP(ray, &enter_time, &exit_time);
        if (!result.happened)
            return result;

        // 如果是透明的材质，交点可能在物体里面，因此就要用离开时间了
        if (enter_time < 0)
        {
            result.coords = Vector3f(ray.origin + ray.direction * exit_time); 
            result.distance = exit_time;
        }
        else 
        { 
            result.coords = Vector3f(ray.origin + ray.direction * enter_time); 
            result.distance = enter_time;
        }
        result.normal = getNormalBySurfacePoint(result.coords);
        result.m = this->m;
        result.obj = this;
        return result;
    }

    Vector3f getNormalBySurfacePoint(const Vector3f& point) const
    {
        const float my_epsilon = EPSILON;
        // 这个计算可能有点抽象，画个pMin在原点时候的一个轴对称矩形
        Vector3f normal_arr[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        Vector3f normal;
        // 不加const会导致链接错误，原因是[]的非const版本确实没有实现，只实现了const的版本
        const Vector3f dmax_vert = point - bound.pMax;    // 用减法判断交点在哪个平面上
        const Vector3f dmin_vert = point - bound.pMin;
        for (int i = 0; i < 3; ++i)
        {
            if (std::fabs(dmax_vert[i]) < my_epsilon)
            {
                normal = normal_arr[i];
                break;
            }
            else if (std::fabs(dmin_vert[i]) < my_epsilon)
            {
                normal = -normal_arr[i];
                break;
            }
        }
        return normal;
    }

    void getSurfaceProperties(const Vector3f &P, const Vector3f &, const uint32_t &, const Vector2f &, Vector3f &N, Vector2f &) const override
    { N = getNormalBySurfacePoint(P); }

    Vector3f evalDiffuseColor(const Vector2f &)const  override{
        return m->getColor();
    }

    Bounds3 getBounds() override{
        return bound;
    }
};

