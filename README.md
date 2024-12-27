+ doc是用typora编写的markdown，可能与github的markdown格式有点不兼容，请使用typora阅读
+ 文档里面有很多latex公式，需要在typora中 文件>偏好设置>Markdown>勾选Markdown扩展语法



## 作业1

<img src="doc/img/作业1截图.JPG" style="zoom:67%;" />

## 作业二

<img src="doc/img/作业2截图.JPG" style="zoom:67%;" />



## 作业三

![](doc/img/games101作业3截图.JPG)



## 作业四

![](doc/img/games101作业4截图.JPG)



## 作业五

![](doc/img/games101作业五截图.JPG)



## 作业六

![](doc/img/games101作业6结果.JPG)



将MeshTriangle::MeshTriangle的bvh = new BVHAccel(ptrs, 1, BVHAccel::SplitMethod::NAIVE);

改成5之后且不开编译优化对比两种划分方法：

+ SAH。跑两次6049ms左右

  <img src="doc/img/bvh_sah_result.JPG" style="zoom:60%;" />

+ NAIVE。跑两次6609ms左右

  <img src="doc/img/bvh_naive_result.JPG" style="zoom:60%;" />

在原来基础上，应该是提升(6609 - 6049) / 6609=8%左右，还是有不错的效果，鉴于我实现的还有些我知道的没有完善的地方。
