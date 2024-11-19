//
// Created by LEI XU on 4/27/19.
//

#include "Texture.hpp"
#include "Eigen/Dense"



static Eigen::Vector3f lerp(float prop, Eigen::Vector3f color0, Eigen::Vector3f color1) 
{
    return color0 + prop * (color1 - color0);
}

Eigen::Vector3f Texture::uvToColor(int u_coordinate, int v_coordinate)
{
    auto c = image_data.at<cv::Vec3b>(u_coordinate, v_coordinate);
    return Eigen::Vector3f(c[0], c[1], c[2]);
}

Eigen::Vector3f Texture::getColorBilinear(float u, float v)
{
    float u_img = u * width;
    float v_img = (1 - v) * height;
    int u_img_min = u_img;
    int v_img_min = v_img;

    // 照着Lecture_09 17-22页，翻译了一下
    // p.s 关于uvToColor的临时对象问题，编译器还是很智能的，开了编译优化没有这个烦恼了，代码还比较好理解
    auto color_u00 = uvToColor(u_img_min, v_img_min);
    auto color_u10 = uvToColor(u_img_min + 1, v_img_min);
    auto color_u01 = uvToColor(u_img_min, v_img_min + 1);
    auto color_u11 = uvToColor(u_img_min + 1, v_img_min + 1);

    float s = u_img - u_img_min;
    float t = v_img - v_img_min;

    auto color_u0 = lerp(s, color_u00, color_u10);
    auto color_u1 = lerp(s, color_u01, color_u11);

    return lerp(t, color_u0, color_u1);
}