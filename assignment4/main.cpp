#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>

std::vector<cv::Point2f> control_points;

void mouse_handler(int event, int x, int y, int flags, void *userdata) 
{
    if (event == cv::EVENT_LBUTTONDOWN && control_points.size() < 4) 
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", "
        << y << ")" << '\n';
        control_points.emplace_back(x, y);
    }     
}

void naive_bezier(const std::vector<cv::Point2f> &points, cv::Mat &window) 
{
    auto &p_0 = points[0];
    auto &p_1 = points[1];
    auto &p_2 = points[2];
    auto &p_3 = points[3];

    for (double t = 0.0; t <= 1.0; t += 0.001) 
    {
        auto point = std::pow(1 - t, 3) * p_0 + 3 * t * std::pow(1 - t, 2) * p_1 +
                 3 * std::pow(t, 2) * (1 - t) * p_2 + std::pow(t, 3) * p_3;

        // 注意opencv的at函数签名是at(i, j)表示i行j列，而opencv的(x, y)左上角是(0, 0)行方向是y方向，列方向是x方向
        // bgr 设置颜色为红色
        window.at<cv::Vec3b>(point.y, point.x)[2] = 255;
    }
}

cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t) 
{
    //  Implement de Casteljau's algorithm
    if (control_points.size() < 2)    // 健壮性，应该控制点不能 < 2
    {
        return cv::Point2f(0, 0);
    }
    if (control_points.size() == 2)
    {
        return (1 - t) * control_points[0] + t * control_points[1];
    }

    std::vector<cv::Point2f> new_control_points;
    for (int i = 1; i < control_points.size(); ++i) 
    {
        auto new_p = (1 - t) * control_points[i - 1]  + t * control_points[i];
        new_control_points.push_back(new_p);
    }
    return recursive_bezier(new_control_points, t);
}

bool color_less(const cv::Vec3b& c1, const cv::Vec3b& c2)
{
    return (c1[0] + c1[1] + c1[2]) < (c2[0] + c2[1] + c2[2]);
}

void anti_aliasing_set_color(cv::Mat& window, cv::Point2f& p)
{
    cv::Vec3b curve_color = {0, 0, 255};
    // https://blog.csdn.net/ycrsw/article/details/124117190
    // 根据p点坐标，画出一个9宫格，p在九宫格的中间格子的某个位置（就是5代表的格子），然后计算九宫格各个的中心坐标到p的距离d，
    // d1表示1这个格子到点的距离
    /*
    1 2 3
    4 5 6
    7 8 9
    */
    // 因此从这个表示中可以知道，每个格子边长为1，d最小为0，最大为3/sqrt(2)，最大情况就是p在5这个格子的右下角，此时的d1就是距离最大
    // 然后定义一下d最小的时候，格子的颜色就是curve_color，即曲线的本来颜色，d最大的时候是0(即window的底色)
    // 然后距离d就是根据这个颜色的最大最小做线性插值
    /*
        颜色     0                 curve_color
        距离  3/sqrt(2)       d        0
    */
    // 类似bezeir，这个比例是r和1-r。很容易就写出来  r = (d - 0) / (3/sqrt(2) - 0)
    // 每个格子的颜色 color = (1 - r) * curve_color = (1 - sqrt(2) / 3 * d) * curve_color
    float coeff = std::sqrt(2) / 3;
    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            int cur_x = p.x + i;
            int cur_y = p.y + j;
            if (cur_x >= window.cols || cur_y >= window.rows)
            {
                continue;
            }

            float cur_mid_x = cur_x + 0.5;
            float cur_mid_y = cur_y + 0.5;
            float d = std::sqrt((cur_mid_x - p.x) * (cur_mid_x - p.x) + (cur_mid_y - p.y) * (cur_mid_y - p.y));
            
            auto cur_color = (1 - coeff * d) * curve_color;
            // 比较大小是为了防止盖住了正常的点，出现暗点
            if (color_less(window.at<cv::Vec3b>(cur_y, cur_x), cur_color))
            {
                window.at<cv::Vec3b>(cur_y, cur_x) = cur_color;
            }
        }
    }
}

void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window) 
{
    // Iterate through all t = 0 to t = 1 with small steps, and call de Casteljau's 
    // recursive Bezier algorithm.
    float t_step = 0.001;
    for (float t = 0.0; t <= 1.0; t = t + t_step)
    {
        auto curve_p = recursive_bezier(control_points, t);
        // window.at<cv::Vec3b>(curve_p.y, curve_p.x)[2] = 255;
        anti_aliasing_set_color(window, curve_p);
    }
}

int main() 
{
    cv::Mat window = cv::Mat(700, 700, CV_8UC3, cv::Scalar(0));
    // cv::cvtColor(window, window, cv::COLOR_BGR2RGB);

    // cvtColor这句转换给注释掉了，因为最后结果发现，bezier曲线还是红色的，at设置的是最后一个分量，好像还是bgr的r。
    // cvtColor生效的话需要保证数据设置好，即放到cv::imshow前面
    // window.at<cv::Vec3b>(point.y, point.x)[2] = 255;
    // 没有仔细去看资料了，凭着实验结果猜测，cvtColor的实现是真的调换通道分量的数值，而不是设置一个解析标记，因为如果设置解析方式的话
    // 在此处cvtColor应该会生效
    cv::namedWindow("Bezier Curve", cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);

    int key = -1;
    while (key != 27) 
    {
        for (auto &point : control_points) 
        {
            cv::circle(window, point, 3, {255, 255, 255}, 3);
        }

        if (control_points.size() == 4) 
        {
            // naive_bezier(control_points, window);
            bezier(control_points, window);    

            cv::imshow("Bezier Curve", window);
            cv::imwrite("my_bezier_curve.png", window);
            key = cv::waitKey(0);

            return 0;
        }

        cv::imshow("Bezier Curve", window);
        key = cv::waitKey(20);
    }

    return 0;
}
