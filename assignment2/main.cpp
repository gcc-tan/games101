// clang-format off
#include <iostream>
#include <opencv2/opencv.hpp>
#include "rasterizer.hpp"
#include "global.hpp"
#include "Triangle.hpp"

constexpr double MY_PI = 3.1415926;

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1,0,0,-eye_pos[0],
                 0,1,0,-eye_pos[1],
                 0,0,1,-eye_pos[2],
                 0,0,0,1;

    view = translate*view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    return model;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio, float zNear, float zFar)
{
    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    // 1.
    // 首先假设（只能是假设）输入参数的语义
    // eye_fov            vertical field-of-view垂直可视角度，假设输入的没有使用弧度
    // aspect_ratio       宽高比 r/t
    // zNear              视锥近平面的距离
    // zFar               视锥远平面的距离
    // 课程上还有个假设，视锥是对称的，即z轴的负半轴穿过了视锥近平面和远平面的中心（利用这个才使用相似三角形进行计算的）

    // 2. 计算l，r，t，b，n，f平面的坐标值
    float l, r, t, b, n, f;
    n = -zNear;
    f = -zFar;
    t = std::fabs(n) * tan (MY_PI / 180 * eye_fov / 2);
    b = -t;
    r = t * aspect_ratio;
    l = -r;

    // 3. 计算persp->ortho 即透视转换投影转换成正交投影的矩阵，在lecture04的36页
    Eigen::Matrix4f m_persp_to_ortho;
    m_persp_to_ortho << n, 0, 0,   0,
                        0, n, 0,   0,
                        0, 0, n+f, -n*f,
                        0, 0, 1,   0;

    // 4. 计算ortho投影矩阵
    Eigen::Matrix4f m_ortho;
    Eigen::Matrix4f m_scale, m_tranlate;
    m_scale <<  2 / (r - l), 0,           0,          0,
                0,           2 / (t -b),  0,          0,
                0,           0,           2 / (n -f), 0,
                0,           0,           0,          1;
    m_tranlate << 1, 0, 0, -(r + l) / 2,
                  0, 1, 0, -(t + b) / 2,
                  0, 0, 1, -(n + f) / 2,
                  0, 0, 0, 1;
    m_ortho = m_scale * m_tranlate;

    projection = m_ortho * m_persp_to_ortho;
    return projection;
}

int main(int argc, const char** argv)
{
    float angle = 0;
    bool command_line = false;
    std::string filename = "output.png";

    if (argc == 2)
    {
        command_line = true;
        filename = std::string(argv[1]);
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0,0,5};


    std::vector<Eigen::Vector3f> pos
            {
                    {2, 0, -2},
                    {0, 2, -2},
                    {-2, 0, -2},
                    {3.5, -1, -5},
                    {2.5, 1.5, -5},
                    {-1, 0.5, -5}
            };

    std::vector<Eigen::Vector3i> ind
            {
                    {0, 1, 2},
                    {3, 4, 5}
            };

    std::vector<Eigen::Vector3f> cols
            {
                    {217.0, 238.0, 185.0},
                    {217.0, 238.0, 185.0},
                    {217.0, 238.0, 185.0},
                    {185.0, 217.0, 238.0},
                    {185.0, 217.0, 238.0},
                    {185.0, 217.0, 238.0}
            };

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);
    auto col_id = r.load_colors(cols);

    int key = 0;
    int frame_count = 0;

    if (command_line)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);   // 这个比较好理解，给出的是rgb，然后opencv实际的输入应该需要bgr

        cv::imwrite(filename, image);

        return 0;
    }

    while(key != 27)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << '\n';
    }

    return 0;
}
// clang-format on