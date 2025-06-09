前面时间比较忙，很久没有打开vscode了。刚打开vscode就说`vscode remote host does not meet prerequisites`。那看样子就只有两种办法了，要么就升级服务器的操作系统。要么就降级这个vscode。本来打算降级一下vscode就行了(1.85版本就行）。找了半天微软的`https://update.code.visualstudio.com/1.85.2/win32-x64-user/stable`vscode打不开，就挂代理也不行奇了怪了。然后各种找镜像，没找到。结果试了一下用迅雷下载可以了....



然后打开了之后又报在remote连接中的XHR错误。XHR应该是http的请求有问题。结果搜了一下。[stack overflow的链接](https://stackoverflow.com/questions/56718453/using-remote-ssh-in-vscode-on-a-target-machine-that-only-allows-inbound-ssh-co/56781109)，说的很清楚就是http下在服务器上下载不了对应的服务器端。

一看下载的链接，又是这个域名：

```
update.code.visualstudio.com/commit:8b3775030ed1a69b13e4f4c628c612102e30a681/server-linux-x64/stable
```

然后感谢这个作者[离线安装 VS Code Server](https://www.cnblogs.com/michaelcjl/p/18262833：)，用下面的域名换成上面的vscode server对应的commit id就好了

```
https://vscode.download.prss.microsoft.com/dbazure/download/stable/${commit_id}/vscode-server-linux-x64.tar.gz
```



> p.s. 现在试又好了。折腾我好久



## opengl的屏幕坐标原点

opengl划线的代码比较简单，如下所示。只需要指定两个顶点的起始位置即可。

```
glBegin(GL_LINES);
   glVertex2d(0, 0);
   glVertex2d(-100, 400);
glEnd();
```
从下图中的效果可以得知，opengl的屏幕坐标(0, 0)的点在这个窗口画布的正中间。从左到右边是x的正方向，从下到上是y的正方向。

<img src="img/opengl_draw_line.JPG" style="zoom:67%;" />



## 常微分方程（Ordinary Differential Equation）

其实不管Euler method（欧拉法）还是Runge-Kutta（龙格-库塔），这些都是数值分析课程里计算常微分方程的一些经典算法。但是看到闫老师的ppt，最需要理解的还是微分方程部分。



![](img/ode_newton.JPG)

个人觉得ppt里面的速度场不好理解。但是可以从物理上给出自己的理解。位移x是时间t的函数，因为给定一个时间一定有唯一的位置x与之对应，即x(t)。这个x(t)，就是要求的运动方程。速度v是t的函数，这个也好理解，物体受外力加速或者减速，反正每个时刻都能求出与t对应的v。由于x(t), v还可以写成v(x, t)的形式。那么x对t微分就是v，即
$$
\frac{dx}{dt} = v(x, t)
$$
一个典型的常微分方程。[显示欧拉，隐式欧拉](https://www.zhihu.com/question/34780726)。相关的说明和对比。



关于[verlet积分法](https://zh.wikipedia.org/wiki/%E9%9F%A6%E5%B0%94%E8%8E%B1%E7%A7%AF%E5%88%86%E6%B3%95)推导和相关计算，就是用泰勒展开。

![](img/verlet_taylor.JPG)



相关的递推公式：



## 代码中的问题

看下面的render代码，render函数是在死循环中一直被调用的。也就是说render函数运行越快，帧率越高。这样其实会导致质点移动过快的鬼畜效果，显得不是很流畅。

```c++

void Application::render() {
  //Simulation loops
  for (int i = 0; i < config.steps_per_frame; i++) {
    ropeEuler->simulateEuler(1 / config.steps_per_frame, config.gravity);
    ropeVerlet->simulateVerlet(1 / config.steps_per_frame, config.gravity);
  }
  // Rendering ropes
 ...
  // 我加上的
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}
```

通过-s设置的steps_per_frame，能够显著增加计算量，降低帧率。那么感觉上就是更丝滑，但其实和simulatexxx函数返回更精确的结果基本没有关系。因为在render里面加上sleep_for函数调用，效果一样....



在隐式欧拉中增加了私货，实现了damping force，停止了沿着弹簧方向的运动。但是垂直弹簧方向没有阻尼，所以会钟摆运动一直下去。

<img src="img/damping_force.JPG" style="zoom:67%;" />
