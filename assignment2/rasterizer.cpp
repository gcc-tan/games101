// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <Eigen/Dense>
#include <algorithm>
#include <limits>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}


static bool insideTriangle(float x, float y, const Vector3f* _v)
{   
    // Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
    // 参考https://blog.csdn.net/dracularking/article/details/2217180 或者课件即可
    Eigen::Vector3f m(x, y, 0);
    Eigen::Vector3f m0 = m - _v[0];
    Eigen::Vector3f m1 = m - _v[1];
    Eigen::Vector3f m2 = m - _v[2];

    // 点v0，v1，v2是顺时针排列，只需要保证，按照顺序一圈下来叉乘的结果同号，那就说明在三角形内部
    // 可以想象一下，m点在三角形内部，那么按照顺时针的顺序的m0，m1，m2叉乘的向量方向肯定是一致的
    float r1 = m0.cross(m1).z();
    float r2 = m1.cross(m2).z();
    float r3 = m2.cross(m0).z();

    // 这里浮点数的比较确实有点害怕
    // https://stackoverflow.com/questions/13767744/detecting-and-adjusting-for-negative-zero
    // https://stackoverflow.com/questions/4915462/how-should-i-do-floating-point-comparison
    if ((r1 > 0 && r2 > 0 && r3 > 0) || (r1 < 0 && r2 < 0 && r3 < 0))
    {
        return true;
    }
    return false;
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
    auto& buf = pos_buf[pos_buffer.pos_id];
    auto& ind = ind_buf[ind_buffer.ind_id];
    auto& col = col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = projection * view * model;
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        //Homogeneous division
        for (auto& vec : v) {
            vec.x() = vec.x() / vec.w();
            vec.y() = vec.y() / vec.w();
            vec.z() = vec.z() / vec.w();
            //vec /= vec.w();
        }
        //Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
            // vert.z() = vert.z() * f1 + f2;
            vert.z() = vert.w();
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        // 这个三角形的颜色不就是纯色的吗？
        // 这个代码用三个顶点三种颜色什么含义？
        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);

        rasterize_triangle(t);

        if (enable_ssaa())
        {
            Eigen::Vector3f pixel_color;
            for (int x = 0; x < width; ++x)
            {
                for (int y = 0; y < height; ++y)
                {
                    int pos = get_index(x, y);
                    pixel_color.setZero();
                    // 计算这个像素内子像素的均值
                    // 说是ssaa，但是我感觉更像msaa https://www.zhihu.com/question/20236638
                    for (const auto& color : ssaa_frame_buf_[pos])
                    {
                        pixel_color += color;
                    }
                    pixel_color /= ssaa_frame_buf_[pos].size();
                    set_pixel(Eigen::Vector3f(x, y, 0), pixel_color);
                }
            }
        }
    }
}

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    auto v = t.toVector4();
    
    // Find out the bounding box of current triangle.
    // iterate through the pixel and find if the current pixel is inside the triangle
    int xmin = std::min({v[0].x(), v[1].x(), v[2].x()});
    int xmax = std::max({v[0].x(), v[1].x(), v[2].x()});
    int ymin = std::min({v[0].y(), v[1].y(), v[2].y()});
    int ymax = std::max({v[0].y(), v[1].y(), v[2].y()});

    for (int x = xmin; x <= xmax; ++x)
    {
        for (int y = ymin; y <= ymax; ++y)
        {
            if (enable_ssaa())
            {
                float distance = 1.0f / (ssaa_size_ + 1);    // 1行ssaa_size_个点需要分成ssaa_size+1段
                for (int i = 0; i < ssaa_size_ * ssaa_size_; ++i)
                {
                    // (x, y)作为像素内左下角的坐标，计算sub_x sub_y即子像素的坐标
                    float sub_x = x + (i % ssaa_size_ + 1) * distance;
                    float sub_y = y + (i / ssaa_size_ + 1) * distance;
                    if (!insideTriangle(sub_x, sub_y, t.v))
                    {
                        continue;
                    }
                    auto[alpha, beta, gamma] = computeBarycentric2D(sub_x, sub_y, t.v);
                    float z_interpolated = 1.0/(alpha / v[0].z() + beta / v[1].z() + gamma / v[2].z()); 

                    int pos = get_index(x, y);
                    if (z_interpolated > ssaa_depth_buf_[pos][i])
                    {
                        // 更新深度值和颜色值
                        ssaa_depth_buf_[pos][i] = z_interpolated;
                        ssaa_frame_buf_[pos][i] = t.getColor();
                    }
                }
            }
            else 
            {
                if (!insideTriangle(x + 0.5, y + 0.5, t.v))
                {
                    continue;
                }
                // If so, use the following code to get the interpolated z value.
                //auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
                //float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                //float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                //z_interpolated *= w_reciprocal;
                // https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/visibility-problem-depth-buffer-depth-interpolation.html
                auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
                float z_interpolated = 1.0/(alpha / v[0].z() + beta / v[1].z() + gamma / v[2].z());


                int pos = get_index(x, y);
                if (z_interpolated > depth_buf[pos])    // 向-z看，因此大的深度值才是靠近相机的
                {
                    depth_buf[pos] = z_interpolated;
                    set_pixel(Eigen::Vector3f(x, y, 0), t.getColor());
                }
            }
        }
    }
}

void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
        if (enable_ssaa())
        {
            for(auto& frame_v : ssaa_frame_buf_)
            {
                frame_v.resize(ssaa_size_ * ssaa_size_);
                std::fill(frame_v.begin(), frame_v.end(), Eigen::Vector3f{0, 0, 0});
            }
        }
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), -std::numeric_limits<float>::infinity());
        if (enable_ssaa())
        {
            for (auto& depth_v : ssaa_depth_buf_)
            {
                depth_v.resize(ssaa_size_ * ssaa_size_);    // 3 * 3的ssaa，每个像素需要扩充为9个点
                std::fill(depth_v.begin(), depth_v.end(), -std::numeric_limits<float>::infinity());
            }
        }
    }
}

rst::rasterizer::rasterizer(int w, int h, int ssaa_size/* = 0 */) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
    ssaa_size_ = ssaa_size;


    if (enable_ssaa())
    {
        ssaa_depth_buf_.resize(w * h);
        ssaa_frame_buf_.resize(w * h);
    }
}

int rst::rasterizer::get_index(int x, int y)
{
    // 转换成一维的方式
    // (x, y)即 viewport transformation中屏幕左下角是原点，向上是y，向右是x，分别对应height，和width
    // 但是考虑到opencv的坐标系中，屏幕的左上角才是坐标原点，因此有了set_pixel函数和这个get_index
    // x = 0， y = height - 1是在viewport transformation坐标系中的左上角，带入此公式验证可以发现get_index返回0
    return (height-1-y)*width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

// clang-format on