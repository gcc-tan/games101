#include <iostream>
#include <Eigen/Dense>    // #include <eigen3/Eigen/Core> 如果仅使用这个头文件，使用叉乘cross这个函数会出现链接错误，因为这是个头文件库
#include <cmath>
#include <vector>

/*
 给定一个点 P =(2,1), 将该点绕原点先逆时针旋转 45◦,再平移 (1,2), 计算出
变换后点的坐标(要求用齐次坐标进行计算)。
 */

Eigen::Matrix3f rotate(double alpha)
{ 
     Eigen::Matrix3f r;
     r << cos(alpha), -sin(alpha), 0,
          sin(alpha), cos(alpha),  0,
          0,             0,        1;
     return r;
}

Eigen::Matrix3f translation(float tx, float ty)
{
     Eigen::Matrix3f t;
     t << 1, 0, tx,
          0, 1, ty,
          0, 0, 1;
     return t;
}

int main()
{
    Eigen::Vector3f p(2.0f, 1.0f, 1.0f);    // p点的齐次坐标
    Eigen::Matrix3f r = rotate(M_PI / 4);    // 构造旋转45度的矩阵
    Eigen::Matrix3f t = translation(1, 2);   // 构造平移矩阵

    // 帮助大家理解Eigen的布局，Eigen固定长度的Vector是只有数据占用的空间，而没有成员变量的
    std::cout << "sizeof Eigen::Vector3f:" << sizeof(Eigen::Vector3f) << std::endl;
    std::cout << "sizeof Eigen::Vector2d:" << sizeof(Eigen::Vector2d) << std::endl;
    
    // 这个例子有助于作业一中convertTo代码的理解
    std::vector<Eigen::Vector3f> frame_buf = { {1.1, 2.2, 3.2}};
    float* test = (float*)frame_buf.data();
    std::cout << "frame_buf value" << std::endl;
    std::cout << test[0] << std::endl;
    std::cout << test[1] << std::endl;
    std::cout << test[2] << std::endl;


    // 加入一个叉乘的例子，如果要计算点c是在ab向量的左侧还是右侧，需要用到叉乘
    // 原理就是计算ca，和cb向量，然后叉乘，如果叉乘向量z为整数，就是在左侧，这个可以用右手定则判定
    // 需要将向量的z坐标置为0，因为本质还是xy平面的二维向量
    std::cout << "============== cross product ==========" << std::endl;
    Eigen::Vector3d a(0, 0, 0), b(2, 2, 0), c(0, 1, 0);
    auto ca = c - a;
    auto cb = c - b;
    std::cout << ca.cross(cb) << std::endl;
    std::cout << "new ca" << std::endl;
    std::cout << ca << std::endl;
    return 0;
}