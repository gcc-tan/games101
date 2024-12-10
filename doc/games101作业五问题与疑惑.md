刚刚做了一个幸福的没有疑惑的作业四，马上又来了作业五，作业五中还是有一些有意思的问题的。



## 计算每个像素的光线方向向量



作业的框架代码原来是这样的，这个光想方向向量dir的计算首先看了两眼觉得有点懵。

```c++
    float scale = std::tan(deg2rad(scene.fov * 0.5f));
    float imageAspectRatio = scene.width / (float)scene.height;

    // Use this variable as the eye position to start your rays.
    Vector3f eye_pos(0);
    int m = 0;
    for (int j = 0; j < scene.height; ++j)
    {
        for (int i = 0; i < scene.width; ++i)
        {
            // generate primary ray direction
            float x;
            float y;
            // Find the x and y positions of the current pixel to get the direction
            // vector that passes through it.
            // Also, don't forget to multiply both of them with the variable *scale*, and
            // x (horizontal) variable with the *imageAspectRatio*            

            Vector3f dir = Vector3f(x, y, -1); // Don't forget to normalize this direction!
        }
    }
```

后面仔细想想才明白什么意思。因为我们知道透视投影中定义一个成像平面至少需要下面这几个参数（相机在原点，-z轴穿过成像平面的中心）：

+ 宽高比，Aspect Ratio。即width / height。
+ 成像平面距离相机的距离，zNear。
+ 视场角，fov。games作业里面用的是垂直fov，即$ tan(\frac{fov}{2}) = \frac {height / 2} {zNear}$

那么对比一下代码，就能知道哪个没有。从dir的z值为-1（dir是从相机开始，连向像素，因此dir的z方向的距离就是相机到成像平面的距离），就能知道zNear=1。



>  其实这个地方建议助教优化一下代码框架，这个写法过于粗糙，也不便于理解和调整参数。
>
> 2024.12.10更新，好吧撤回一部分。仔细看下面计算，w和h都是通过zNear计算出来的，最后单位化这个向量，zNear会约分约掉。最后计算光线方向向量是一样的。（可以想像一个视锥的最外面光线，不管你怎么移动近平面，都会不变）



有了定义参数了，就需要通过（i，j）来计算x和y。画画图也可以直接写出。但是我们用一个**更加高级的整活方法**。就是我在《view变换》中归纳总结的一般性的问题，即坐标系变换。



在坐标系变换前，需要计算一下投影平面的实际宽w和高h。根据定义，很容易写出来
$$
因为\frac{h/2}{zNear} = tan(fov/2)
\\
所以：h = 2*zNear*tan(fov/2)
\\
w = aspect\_ratio * 2*zNear*tan(fov/2)
$$


原有的坐标系命名为A。这个时候建立一个新的坐标系，命名为B。B坐标系的原点在成像平面的左上角，B的x轴为A坐标系的x轴，B的y轴为A的y轴反方向。p点就是i，j代表的点。如下图灵魂画手所示：

![](img/simple_coordinates_trans.JPG)

那么，此时就是已知$P_B$（点P在B坐标系下的坐标）。要求$P_A$。



已知条件：
$$
P_B = (\frac{i}{width} * w, \frac{j}{height} * h)
\\
(\vec{O_1O_2})_A = (-w/2, h/2)
$$
根据《view变换》中的说明$P_A$，满足如下公式
$$
P_A = \sideset{^A_B}{}RP_B + (\vec{O_1O_2})_A
$$
那么只需要求出，$\sideset{^A_B}{}R$。这个矩阵是一个旋转矩阵，参考《view变换》，只要求出B的两个坐标轴在A下的坐标表示，就能直接写出这个旋转矩阵。第一列就是上图中的$x_B$单位向量在A坐标系的表示，第二列就是$y_B$单位向量在坐标系A的表示
$$
\sideset{^A_B}{}R = 
\begin{bmatrix}
1 &  0 \\
0 & -1  \\
\end{bmatrix}
$$

$$

$$
因此
$$
P_A =
\begin{bmatrix}
1 &  0 \\
0 & -1  \\
\end{bmatrix}
*
\begin{bmatrix}
\frac{i}{width} * w  \\
\frac{j}{height} * h  \\
\end{bmatrix}
+
\begin{bmatrix}
 -w / 2  \\
 h / 2  \\
\end{bmatrix}
\\
=
\begin{bmatrix}
 \frac{i}{width} * w - w / 2  \\
 -\frac{j}{height} * h + h / 2  \\
\end{bmatrix}
$$


到这里，大家可以带入i和j的特殊值试下，这个计算是没有问题的。例如i=0，j=0时候，P在左上角的B坐标系原点。算出来，然后i=width/2，j=height/2，P在A坐标系的原点时。





> 实现作业五的时候，发现一片蓝色。也就是光线与物体完全没有相交。因为这个代码中要实现的地方只有两处，一处是生成光线方向向量，一处是使用Moller Tumbore算法判断判断光线与三角形的交点。然后在Moller Tumbore处打印日志，发现t全是负数。以为是这个Moller算法实现问题。把别人正确的代码替换之后还是不行。因此就看下生成光线的代码，发现$P_A$的y坐标少了个负号。。。。 p.s sort按照某个字段进行排序时，需要使用-k，而且是 -k 2,2 这种才是按照第二个字段进行排序
