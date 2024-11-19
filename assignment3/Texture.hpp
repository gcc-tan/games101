//
// Created by LEI XU on 4/27/19.
//

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H
#include "global.hpp"
#include <eigen3/Eigen/Eigen>
#include <opencv2/opencv.hpp>
class Texture{
private:
    cv::Mat image_data;

public:
    Texture(const std::string& name, bool use_binear=false)
    {
        image_data = cv::imread(name);
        cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
        width = image_data.cols;
        height = image_data.rows;

        if (use_binear)
        {
            std::cout << "use binear" << std::endl;
        }
    }

    int width, height;
    bool use_binear;

    Eigen::Vector3f getColor(float u, float v)
    {
        if (use_binear)
        {
            return getColorBilinear(u, v);
        }
        auto u_img = u * width;
        auto v_img = (1 - v) * height;
        auto color = image_data.at<cv::Vec3b>(v_img, u_img);
        return Eigen::Vector3f(color[0], color[1], color[2]);
    }

    // 双线性插值
    Eigen::Vector3f getColorBilinear(float u, float v);
private:
    Eigen::Vector3f uvToColor(int u_coordinate, int v_coordinate);
};
#endif //RASTERIZER_TEXTURE_H
