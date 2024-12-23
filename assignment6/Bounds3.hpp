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

    inline bool IntersectP(const Ray& ray, float* enter_time = nullptr) const;
};


bool Bounds3::IntersectP(const Ray& ray, float* enter_time/*= nullptr*/) const
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
     * 
     * 以计算x对面为例，x对面的法线nx是(1, 0, 0)，可以化简并直接写出上面的表达式
     * 
     * 之前以为dirIsNeg没用，后面发现运行后看不到兔子....
     * dirIsNeg的作用在于计算tmin和tmax，因为tmin表示进入对面的时间，tmax表出对面的时间
     * 因为pMin表示的是下对面，pMax表示的是上对面。以x对面为例子，d的x分量如果是负数，那么表示的情况就是光线从上往下走
     * 这种情况利用pMax算出来的是进入对面的时间tmin，与x是正的情况相反
     */ 
    const auto& invDir = ray.direction_inv;
    const auto& dirIsNeg = ray.DirIsNeg();
    float t_enter = std::numeric_limits<float>::lowest(), t_exit = std::numeric_limits<float>::max();
    float t_min, t_max;
    for (int i = 0; i < 3; ++i)
    {
        t_min = (pMin[i] - ray.origin[i]) * invDir[i];
        t_max = (pMax[i] - ray.origin[i]) * invDir[i];
        if (dirIsNeg[i])
            std::swap(t_min, t_max);
        t_enter = std::max(t_enter, t_min);
        t_exit = std::min(t_exit, t_max);
    }

    if (enter_time)
        *enter_time = t_enter;

    return t_enter < t_exit && t_enter >= 0;    // 这个大于等于0还是不能少的，因为会出现包围盒在d的反方向上
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
