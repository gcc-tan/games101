#include <iostream>
#include <eigen3/Eigen/Core>
#include <cmath>

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

    std::cout << t * r * p  << std::endl;
    return 0;
}