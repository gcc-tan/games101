//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_INTERSECTION_H
#define RAYTRACING_INTERSECTION_H
#include "Vector.hpp"
#include "Material.hpp"
class Object;
class Sphere;

struct Intersection
{
    Intersection(){
        happened=false;
        coords=Vector3f();
        normal=Vector3f();
        distance= std::numeric_limits<double>::max();
        obj =nullptr;
        m=nullptr;
    }
    bool happened;
    Vector3f coords;   // 光线与物体交点坐标
    Vector3f normal;   // 光线与物体交点的法线，击中的物体是三角形，就用面法线
    double distance;   // 光线起点到交点距离
    Object* obj;
    Material* m;
};
#endif //RAYTRACING_INTERSECTION_H
