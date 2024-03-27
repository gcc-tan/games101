#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <eigen3/Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <cmath>

constexpr double MY_PI = 3.1415926;

// 不太明白为什么只要讲相机移动到原点即可，不是说相机要朝-z看吗？为什么没有对look at direction进行调整？
Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
        -eye_pos[2], 0, 0, 0, 1;

    view = translate * view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the model matrix for rotating the triangle around the Z axis.
    // Then return it.
    double a = rotation_angle / 180 * MY_PI;    // 假设输入的是角度不是弧度
    model << cos(a), -sin(a), 0, 0,
             sin(a), cos(a),  0, 0,
             0,           0,  1, 0,
             0,           0,  0, 1;
             

    return model;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    // Students will implement this function

    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the projection matrix for the given parameters.
    // Then return it.
    
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

    if (argc >= 3) {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4) {
            filename = std::string(argv[3]);
        }
        else
            return 0;
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0, 0, 5};

    std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

    std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);

    int key = 0;
    int frame_count = 0;

    if (command_line) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());    // 这段代码很精妙。 CV_32FC3,的空间正好对应Eigen::Vector3f
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    while (key != 27) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << '\n';

        if (key == 'a') {
            angle += 10;
        }
        else if (key == 'd') {
            angle -= 10;
        }
    }

    return 0;
}
