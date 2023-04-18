
## 简单视频播放器

    - 摄像头视频流显示
    - 视频播放

------------------

工具：

QT + ffmpeg + cmake

子线程解码，主线程定时播放，利用双缓冲队列，根据时间戳音频同步视频

* 记得把bin目录加入环境变量

---

#### TODO
    功能
        - 粘贴路径
        - 对应选择待播放文件 可以增加按钮
        - 增加摄像头的选择、多个摄像头显示
        - 网络视频流传输
        - 自适应屏幕大小

    内部实现
        - 一个线程解视频，一个线程解音频(目前音频较卡)
        - 存储日志，选择输出用户
        - 流数据与普通视频文件区分
        - 有的视频的pts有问题，没加入对应方案
        - 加入一点图像处理


---

ref:\
[关于avcodec_parameters_to_context](https://stackoverflow.com/questions/39105571/decoding-mp4-mkv-using-ffmpeg-fails)\
[avcodec_parameters_to_context](avcodec_parameters_to_context)\
[雷老师的视频](https://www.bilibili.com/video/BV14x411D7FD/?spm_id_from=333.337.search-card.all.click&vd_source=46eebb652d0c3c3987da6af82b7e1c59)\
[雷老师的csdn](https://blog.csdn.net/leixiaohua1020?type=blog)\
[ffmpeg新旧接口对照表](https://www.cnblogs.com/schips/p/12197116.html)\
[QAudioOutput类使用](https://www.cnblogs.com/lifexy/p/13648303.html)\
[使用QAudioOutput播放ffmpeg解码出的音频](https://wobushixiaohai.blog.csdn.net/article/details/117605090?spm=1001.2101.3001.6661.1&utm_medium=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7ECTRLIST%7EPayColumn-1-117605090-blog-117548649.235%5Ev28%5Epc_relevant_3mothn_strategy_recovery&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7ECTRLIST%7EPayColumn-1-117605090-blog-117548649.235%5Ev28%5Epc_relevant_3mothn_strategy_recovery&utm_relevant_index=1)\
[基于FFMPEG设计视频播放器-软解图像](https://blog.csdn.net/xiaolong1126626497/article/details/126832537?ydreferer=aHR0cHM6Ly9pLmNzZG4ubmV0Lw%3D%3D)\
[FFmpeg 开发(06)：FFmpeg 播放器实现音视频同步的三种方式](https://zhuanlan.zhihu.com/p/219968230)\
[Qt-FFmpeg开发-打开本地摄像头](https://blog.csdn.net/qq_43627907/article/details/128184141?spm=1001.2014.3001.5506)
