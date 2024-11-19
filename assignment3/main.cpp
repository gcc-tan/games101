#include <iostream>
#include <opencv2/opencv.hpp>

#include "global.hpp"
#include "rasterizer.hpp"
#include "Triangle.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "OBJ_Loader.h"

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

Eigen::Matrix4f get_model_matrix(float angle)
{
    Eigen::Matrix4f rotation;
    angle = angle * MY_PI / 180.f;
    // y轴旋转
    rotation << cos(angle), 0, sin(angle), 0,
                0, 1, 0, 0,
                -sin(angle), 0, cos(angle), 0,
                0, 0, 0, 1;

    // 放大2.5倍
    Eigen::Matrix4f scale;
    scale << 2.5, 0, 0, 0,
              0, 2.5, 0, 0,
              0, 0, 2.5, 0,
              0, 0, 0, 1;

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1;

    return translate * rotation * scale;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio, float zNear, float zFar)
{
    // TODO: Use the same projection matrix from the previous assignments
    float l, r, t, b, n, f;
    n = -zNear;
    f = -zFar;
    t = std::fabs(n) * tan(MY_PI / 180 * eye_fov / 2);
    b = -t;
    r = t * aspect_ratio;
    l = -r;

    Eigen::Matrix4f projection;
    projection << 2*n / (r - l), 0,             0,                 0,
                  0,             2*n / (t - b), 0,                 0,
                  0,             0,             (n + f) / (n - f), -2*n*f / (n -f),
                  0,             0,             1,                 0;

    return projection;
}

Eigen::Vector3f vertex_shader(const vertex_shader_payload& payload)
{
    return payload.position;
}

Eigen::Vector3f normal_fragment_shader(const fragment_shader_payload& payload)
{
    // 这个shader还是有点不太能理解干什么
    // 针对shader这一点的法线 + (1, 1, 1)，理解是直接将法线x, y, z变成正的。这个/2是干啥？平均啊
    // 然后直接将这个结果的三个分量作为颜色
    // 能想到的就是深度图类似的操作，并没有什么实际意义？
    Eigen::Vector3f return_color = (payload.normal.head<3>().normalized() + Eigen::Vector3f(1.0f, 1.0f, 1.0f)) / 2.f;
    Eigen::Vector3f result;
    result << return_color.x() * 255, return_color.y() * 255, return_color.z() * 255;
    return result;
}

static Eigen::Vector3f reflect(const Eigen::Vector3f& vec, const Eigen::Vector3f& axis)
{
    auto costheta = vec.dot(axis);
    return (2 * costheta * axis - vec).normalized();
}

struct light
{
    Eigen::Vector3f position;
    Eigen::Vector3f intensity;
};

Eigen::Vector3f texture_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f return_color = {0, 0, 0};
    if (payload.texture)
    {
        // TODO: Get the texture value at the texture coordinates of the current fragment
        auto tp = payload.tex_coords;
        return_color = payload.texture->getColor(tp.x(), tp.y());
    }
    Eigen::Vector3f texture_color;
    texture_color << return_color.x(), return_color.y(), return_color.z();

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = texture_color / 255.f;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = texture_color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};
    Eigen::Vector3f Ld, Ls, La;
    Eigen::Vector3f h, l, v;
    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *result_color* object.
        v = (eye_pos - point).normalized();
        l = (light.position - point).normalized();
        h = (v + l).normalized();

        float rr = (light.position - point).squaredNorm();    // shading point和光源距离的平方
        Ld = kd.cwiseProduct(light.intensity) / rr * std::max(0.f, normal.dot(l));
        Ls = ks.cwiseProduct(light.intensity) / rr * std::pow(std::max(0.f, normal.dot(h)), p);
        La = ka.cwiseProduct(amb_light_intensity);

        result_color = result_color + Ld + Ls + La;
    }

    return result_color * 255.f;
}

Eigen::Vector3f phong_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    // 两个光源，应该是view space的坐标
    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};    // 此处的eye_pos不应该就是相机坐标，那在view space，已经是(0, 0, 0)了？

    float p = 150;

    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};
    Eigen::Vector3f Ld, Ls, La;    // 漫反射光，高光，环境光
    Eigen::Vector3f h, l, v;    // 半程向量
    for (auto& light : lights)
    {
        v = (eye_pos - point).normalized();
        l = (light.position - point).normalized();
        h = (v + l).normalized();
        float rr = (point - light.position).squaredNorm();
        // (point - light.position).norm() shading point的欧式距离
        Ld = kd.cwiseProduct(light.intensity) / rr * std::max(0.f, normal.dot(l));
        Ls = ks.cwiseProduct(light.intensity) / rr * std::pow(std::max(0.f, normal.dot(h)), p);
        La = ka.cwiseProduct(amb_light_intensity);

        result_color = result_color + Ld + Ls + La;
    }

    return result_color * 255.f;
}


// displacement mapping是位移贴图，bump mapping是凹凸贴图。虽然两者都是利用同样的高度图计算法线
// 但是凹凸贴图只是改变了法线从而改变光照结果，不改变真实的顶点位置（这样可能导致边缘，阴影，和凹凸部分间遮挡的着色可能不正确）
// 而displacement mapping是改变法线的同时改变了顶点位置，这样很减少或者避免上面括号中的问题
// normal mapping和bump mapping 的区别呢？
// 个人认为只是从贴图中获取法线的方式不一样，normal mapping是从贴图中利用颜色做个线性变换就能获取，bump mapping还需要算差分，算切线。最后利用法线做的操作一样。
Eigen::Vector3f displacement_fragment_shader(const fragment_shader_payload& payload)
{
    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color; 
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    float kh = 0.2, kn = 0.1;
    // Let n = normal = (x, y, z)
    // Vector t = (x*y/sqrt(x*x+z*z),sqrt(x*x+z*z),z*y/sqrt(x*x+z*z))
    // Vector b = n cross product t
    // Matrix TBN = [t b n]
    // dU = kh * kn * (h(u+1/w,v)-h(u,v))
    // dV = kh * kn * (h(u,v+1/h)-h(u,v))
    // Vector ln = (-dU, -dV, 1)
    // Position p = p + kn * n * h(u,v)
    // Normal n = normalize(TBN * ln)

    // 计算tbn矩阵
    float x = payload.normal.x();
    float y = payload.normal.y();
    float z = payload.normal.z();
    float proj_xz_len = sqrt(x*x + z*z);
    Eigen::Vector3f t(- y * x / proj_xz_len, proj_xz_len, -y * z / proj_xz_len); 
    Eigen::Vector3f b = payload.normal.cross(t);
    Eigen::Matrix3f tbn;
    tbn << t, b, payload.normal;

    float u = payload.tex_coords.x();
    float v = payload.tex_coords.y();
    float w = payload.texture->width;
    float h = payload.texture->height;
    float h_uv = payload.texture->getColor(u, v).norm();    
    float dU = kh * kn * (payload.texture->getColor(u + 1 / w, v).norm() - h_uv);    
    float dV = kh * kn * (payload.texture->getColor(u, v + 1 / h).norm() - h_uv);
    Eigen::Vector3f ln(-dU, -dV, 1);

    point = point + kn * normal * h_uv;    // 看起来就是着色点沿着旧法线移动纹理贴图中
    Eigen::Vector3f n = (tbn * ln).normalized();

    


    Eigen::Vector3f result_color = {0, 0, 0};
    Eigen::Vector3f Ld, Ls, La;    // 漫反射光，高光，环境光
    Eigen::Vector3f h_vec, l_vec, v_vec;    // 半程向量
    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *result_color* object.
        v_vec = (eye_pos - point).normalized();
        l_vec = (light.position - point).normalized();
        h_vec = (v_vec + l_vec).normalized();
        float rr = (point - light.position).squaredNorm();
        // (point - light.position).norm() shading point的欧式距离
        Ld = kd.cwiseProduct(light.intensity) / rr * std::max(0.f, n.dot(l_vec));
        Ls = ks.cwiseProduct(light.intensity) / rr * std::pow(std::max(0.f, n.dot(h_vec)), p);
        La = ka.cwiseProduct(amb_light_intensity);

        result_color = result_color + Ld + Ls + La;
    }

    return result_color * 255.f;
}


Eigen::Vector3f bump_fragment_shader(const fragment_shader_payload& payload)
{
    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    float kh = 0.2, kn = 0.1;

    // Let n = normal = (x, y, z)
    // Vector t = (x*y/sqrt(x*x+z*z),sqrt(x*x+z*z),z*y/sqrt(x*x+z*z))
    // Vector b = n cross product t
    // Matrix TBN = [t b n]
    // https://games-cn.org/forums/topic/zuoye3-bump-mappingzhongtbndet-gongshizenmetuidaode/

    // 计算tbn矩阵
    float x = payload.normal.x();
    float y = payload.normal.y();
    float z = payload.normal.z();
    float proj_xz_len = sqrt(x*x + z*z);
    Eigen::Vector3f t(- y * x / proj_xz_len, proj_xz_len, -y * z / proj_xz_len);    // 为什么这么算在《games101作业三问题与疑惑中介绍》
    Eigen::Vector3f b = payload.normal.cross(t);    // 注意顺序 n x t
    Eigen::Matrix3f tbn;
    /* 这个初始化相当于
    tbn << t.x(), b.x(), payload.normal.x(),
           t.y(), b.y(), payload.normal.y(),
           t.z(), b.z(), payload.normal.z()
    */
    tbn << t, b, payload.normal;

    float u = payload.tex_coords.x();
    float v = payload.tex_coords.y();
    float w = payload.texture->width;
    float h = payload.texture->height;
    // 好像是论坛助教解答问题说的:color的模表示h(u, v)即(u, v)这点的高度值
    // https://games-cn.org/forums/topic/%E4%BD%9C%E4%B8%9A3%E6%9B%B4%E6%AD%A3%E5%85%AC%E5%91%8A/
    float h_uv = payload.texture->getColor(u, v).norm();    
    float dU = kh * kn * (payload.texture->getColor(u + 1 / w, v).norm() - h_uv);    // 因为u是已经/w的值，因此用差分移动一个单位是1/w
    float dV = kh * kn * (payload.texture->getColor(u, v + 1 / h).norm() - h_uv);
    Eigen::Vector3f ln(-dU, -dV, 1);
    Eigen::Vector3f n = (tbn * ln).normalized();

    Eigen::Vector3f result_color = {0, 0, 0};
    result_color = n;

    return result_color * 255.f;
}

int main(int argc, const char** argv)
{
    std::vector<Triangle*> TriangleList;

    float angle = 140.0;
    bool command_line = false;

    std::string filename = "output.png";
    objl::Loader Loader;
    std::string obj_path = "../models/spot/";

    // Load .obj File
    bool loadout = Loader.LoadFile("../models/spot/spot_triangulated_good.obj");
    for(auto mesh:Loader.LoadedMeshes)
    {
        for(int i=0;i<mesh.Vertices.size();i+=3)
        {
            Triangle* t = new Triangle();
            for(int j=0;j<3;j++)
            {
                t->setVertex(j,Vector4f(mesh.Vertices[i+j].Position.X,mesh.Vertices[i+j].Position.Y,mesh.Vertices[i+j].Position.Z,1.0));
                t->setNormal(j,Vector3f(mesh.Vertices[i+j].Normal.X,mesh.Vertices[i+j].Normal.Y,mesh.Vertices[i+j].Normal.Z));
                t->setTexCoord(j,Vector2f(mesh.Vertices[i+j].TextureCoordinate.X, mesh.Vertices[i+j].TextureCoordinate.Y));
            }
            TriangleList.push_back(t);
        }
    }

    rst::rasterizer r(700, 700);

    auto texture_path = "hmap.jpg";    // 之前做纹理映射的是后忘记改了，导致做这个凹凸贴图的时候结果很有意思，过于光滑，可以试试
    r.set_texture(Texture(obj_path + texture_path));

    std::function<Eigen::Vector3f(fragment_shader_payload)> active_shader = texture_fragment_shader;

    if (argc >= 2)
    {
        command_line = true;
        filename = std::string(argv[1]);

        if (argc >= 3 && std::string(argv[2]) == "texture")
        {
            std::cout << "Rasterizing using the texture shader\n";
            active_shader = texture_fragment_shader;
            texture_path = "spot_texture.png";
            r.set_texture(Texture(obj_path + texture_path, argc > 3));
        }
        else if (argc == 3 && std::string(argv[2]) == "normal")
        {
            std::cout << "Rasterizing using the normal shader\n";
            active_shader = normal_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "phong")
        {
            std::cout << "Rasterizing using the phong shader\n";
            active_shader = phong_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "bump")
        {
            std::cout << "Rasterizing using the bump shader\n";
            active_shader = bump_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "displacement")
        {
            std::cout << "Rasterizing using the displacement shader\n";
            active_shader = displacement_fragment_shader;
        }
    }

    Eigen::Vector3f eye_pos = {0,0,10};

    r.set_vertex_shader(vertex_shader);
    r.set_fragment_shader(active_shader);

    int key = 0;
    int frame_count = 0;

    if (command_line)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);
        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));

        r.draw(TriangleList);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imwrite(filename, image);

        return 0;
    }

    while(key != 27)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));

        //r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);
        r.draw(TriangleList);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imshow("image", image);
        cv::imwrite(filename, image);
        key = cv::waitKey(10000);    // 这个等待时间要更长一点，如果不在这个时间内按下，那会返回-1，导致模型旋转不了

        if (key == 'a' )
        {
            angle -= 10;
        }
        else if (key == 'd')
        {
            angle += 10;
        }

    }
    return 0;
}
