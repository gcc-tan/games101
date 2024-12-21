//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_BOUNDS3_H
#define RAYTRACING_BOUNDS3_H
#include "Ray.hpp"
#include "Vector.hpp"
#include <limits>
#include <array>

class Bounds3
{
  public:
    // 轴对称的包围盒，可以用对角的两个点表示，可以理解为两组x，两组y，两组进行包围
    Vector3f pMin, pMax; // two points to specify the bounding box
    Bounds3()
    {
        double minNum = std::numeric_limits<double>::lowest();    // 对于double来说，lowest是负的，min会返回最接近于0的正数
        double maxNum = std::numeric_limits<double>::max();
        // pMax取最小，pMin选最大，表示未初始化的情况，从而保证Union(未初始化的bound，已初始化的bound)=已经初始化的bound
        pMax = Vector3f(minNum, minNum, minNum);    
        pMin = Vector3f(maxNum, maxNum, maxNum);
    }
    Bounds3(const Vector3f p) : pMin(p), pMax(p) {}
    Bounds3(const Vector3f p1, const Vector3f p2)
    {
        pMin = Vector3f(fmin(p1.x, p2.x), fmin(p1.y, p2.y), fmin(p1.z, p2.z));
        pMax = Vector3f(fmax(p1.x, p2.x), fmax(p1.y, p2.y), fmax(p1.z, p2.z));
    }

    Vector3f Diagonal() const { return pMax - pMin; }
    int maxExtent() const
    {
        Vector3f d = Diagonal();
        if (d.x > d.y && d.x > d.z)
            return 0;
        else if (d.y > d.z)
            return 1;
        else
            return 2;
    }

    double SurfaceArea() const
    {
        Vector3f d = Diagonal();
        return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
    }

    Vector3f Centroid() { return 0.5 * pMin + 0.5 * pMax; }
    Bounds3 Intersect(const Bounds3& b)
    {
        // 假设有重叠的情况，长方体包围盒两个点，min点里面选最大的，max点里面选最小的
        // todo？可以用Vector的Min和Max函数，代码看起来更简单一点
        return Bounds3(Vector3f(fmax(pMin.x, b.pMin.x), fmax(pMin.y, b.pMin.y),
                                fmax(pMin.z, b.pMin.z)),
                       Vector3f(fmin(pMax.x, b.pMax.x), fmin(pMax.y, b.pMax.y),
                                fmin(pMax.z, b.pMax.z)));
    }

    Vector3f Offset(const Vector3f& p) const
    {
        Vector3f o = p - pMin;
        if (pMax.x > pMin.x)
            o.x /= pMax.x - pMin.x;
        if (pMax.y > pMin.y)
            o.y /= pMax.y - pMin.y;
        if (pMax.z > pMin.z)
            o.z /= pMax.z - pMin.z;
        return o;
    }

    // 判断是否重叠，画出平面的情况就比较好理解
    // 不过这个语义有点诡异，没有使用任何成员变量，全靠参数输入
    // 这种情况不声明为静态函数，或者像Union一样放到类外面去
    bool Overlaps(const Bounds3& b1, const Bounds3& b2)
    {
        bool x = (b1.pMax.x >= b2.pMin.x) && (b1.pMin.x <= b2.pMax.x);
        bool y = (b1.pMax.y >= b2.pMin.y) && (b1.pMin.y <= b2.pMax.y);
        bool z = (b1.pMax.z >= b2.pMin.z) && (b1.pMin.z <= b2.pMax.z);
        return (x && y && z);
    }

    bool Inside(const Vector3f& p, const Bounds3& b)
    {
        return (p.x >= b.pMin.x && p.x <= b.pMax.x && p.y >= b.pMin.y &&
                p.y <= b.pMax.y && p.z >= b.pMin.z && p.z <= b.pMax.z);
    }
    inline const Vector3f& operator[](int i) const
    {
        return (i == 0) ? pMin : pMax;
    }

    inline bool IntersectP(const Ray& ray, const Vector3f& invDir,
                           const std::array<int, 3>& dirisNeg) const;
};


inline bool Bounds3::IntersectP(const Ray& ray, const Vector3f& invDir,
                                const std::array<int, 3>& dirIsNeg) const
{
    // invDir: ray direction(x,y,z), invDir=(1.0/x,1.0/y,1.0/z), use this because Multiply is faster that Division
    // dirIsNeg: ray direction(x,y,z), dirIsNeg=[int(x>0),int(y>0),int(z>0)], use this to simplify your logic
    // test if ray bound intersects
    /*
     * 根据Lecture13 ppt的28页平面的点法式方程和37-38页，AABB包围盒与光线相交的计算思路
     * 原来计算t的方程是：
     *   t = {(p' - o) dot_product N } / { d dot_product N }
     *   其实这个方程也好理解点乘就是计算投影的长度，分子的点乘就是op'向量在N的投影
     *   即表示的就是o到p'坐在平面的垂直距离，d是光线的方向向量，是个单位向量。可以理解为速度向量
     *   因为光线方程就是o + td，因此单位的速度向量表示单位时间走的方向和距离
     *   所以分母的 d dot_product N 表示速度向量在垂直平面方向上的投影，即单位时间走的距离
     *   两者相除就是时间
     */ 
    // 先计算x对面，x对面的法线nx是(1, 0, 0)，可以化简并直接写出上面的表达式
    float tx_min = (pMin.x - ray.origin.x) * invDir.x;
    float tx_max = (pMax.x - ray.origin.x) * invDir.x;
    float ty_min = (pMin.y - ray.origin.y) * invDir.y;
    float ty_max = (pMax.y - ray.origin.y) * invDir.y;
    float tz_min = (pMin.z - ray.origin.z) * invDir.z;
    float tz_max = (pMax.z - ray.origin.z) * invDir.z;

    float t_enter = std::max({tx_min, ty_min, tz_min});
    float t_exit = std::min({tx_max, ty_max, tz_max});
    return t_enter < t_exit;
}

inline Bounds3 Union(const Bounds3& b1, const Bounds3& b2)
{
    Bounds3 ret;
    ret.pMin = Vector3f::Min(b1.pMin, b2.pMin);
    ret.pMax = Vector3f::Max(b1.pMax, b2.pMax);
    return ret;
}

inline Bounds3 Union(const Bounds3& b, const Vector3f& p)
{
    Bounds3 ret;
    ret.pMin = Vector3f::Min(b.pMin, p);
    ret.pMax = Vector3f::Max(b.pMax, p);
    return ret;
}

#endif // RAYTRACING_BOUNDS3_H
