#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QTimerEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE


#include <QDebug>
#include <QFileDialog>
#include <QString>
#include <QImage>
#include <QPixmap>
#include <QAudioOutput>
#include <QThread>
#include <QReadWriteLock>
#include <QMutexLocker>
#include <QDateTime>
#include <QQueue>
#include <QCameraInfo>

#include <iostream>
#include <string>
using namespace std;

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
#include <libswresample/swresample.h>
#include "libavdevice/avdevice.h"
}

//#include <pthread.h>
//#include <time.h>

class Read_frame;


class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    void init();
    bool init_decoder();    //初始化与释放在主线程
    void free_decoder();

    void video_play();      //显示
    void audio_play();      //播放

    void show_error(int);

private:
    Ui::Widget *ui;

    QTimer* timer_pic;  //图像
    QTimer* timer_aud;  //音频

public:
    //状态机实现功能切换
    enum PLAY_STATUS{
        CHOSE_FILE = 0,
        START,
        PAUSE,
        STOP,
        CONTINUE,
        END,
        BAD         //读取过程中发生错误 未使用
    };

    string file_name;  //文件名
    PLAY_STATUS pls;    //状态量
    FILE* fd;           //cpp文件描述符
    double speed;

    ///解码相关
    bool parsed;        //是否解析完毕

    //流相关
    const AVInputFormat* in_format; //流相关
    AVDictionary* dict;

    AVFormatContext* format_context;

    //视频
    int videoindex;
    const AVCodec* codec;
    AVCodecContext* codec_context;
    AVPacket* pkt;
    AVFrame* frame;
    SwsContext* sw_context;
    uint8_t* out_buf[4];
    int out_stride[4];
    int64_t video_pts;  //初始化

    struct video_buf
    {
        uint8_t* out_buf;
        int64_t pts;
    };
    QQueue<video_buf> video_buf_arr;  //初始化 释放

    //音频
    int audioindex;
    const AVCodec* au_coder;
    AVCodecContext* au_code_context;
    SwrContext* swr_ctx;
    QAudioFormat audioFormat;  //音频播放格式
    QAudioOutput* audioOutput;  //播放
    QIODevice *audioDevice;     //播放设备

    struct audio_buf
    {
        int bufsize;
        uint8_t *read_buf;
        int64_t pts;
    };
    QQueue<audio_buf> audio_buf_arr;  //初始化 释放

    //解码类
    Read_frame* read_t;

    //锁
    QMutex lock_pix;
    QMutex lock_audio;

    int64_t pic_dur;    //视频持续时间
    int64_t aud_dur;


signals:
    void you_can_start(Widget*);    //发送解码数据
    void can_continue();            //告诉线程可以继续工作
};


//解码类
class Read_frame : public QThread
{
    Q_OBJECT

public:
    explicit Read_frame(QObject *parent = nullptr);
    ~Read_frame();

    void run() override;

public:
    void receive_info(Widget*);
    void receive_run();

private:
    bool stop;      //0 允许解码
    Widget* wgt;    //只允许改动解码部分 状态量 缓存数据


signals:
    void status(bool);  //解析完毕

};


#endif // WIDGET_H
