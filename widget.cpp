#include "widget.h"
#include "./ui_widget.h"


//////////任务类///////////

Read_frame::Read_frame(QObject* parent): QThread(parent)
{
    stop = true;
    wgt = nullptr;
}

Read_frame::~Read_frame()
{

}

void Read_frame::run()
{
    while(!isInterruptionRequested())
    {
        if(!stop)
        {
//            qDebug()<<"接收信号"<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
//            qDebug()<<"...";
            int ret;
            ret = av_read_frame(wgt->format_context, wgt->pkt);
            if(ret<0)
            {
                perror("av_read_frame");
//                emit status(Widget::END);  //现在不是解析完就end了，改成读完缓冲队列
                emit status(1);
                return;
            }

            if(wgt->pkt->stream_index==wgt->videoindex)
            {
                ret = avcodec_send_packet(wgt->codec_context, wgt->pkt);
                ret = avcodec_receive_frame(wgt->codec_context, wgt->frame);
                if(ret==0)
                {
//                    cout<<"time_base:"<<1000*wgt->format_context->streams[wgt->videoindex]->time_base.num<<" / "<<wgt->format_context->streams[wgt->videoindex]->time_base.den<<endl;
//                    int num = wgt->format_context->streams[wgt->videoindex]->time_base.num;
//                    int den = wgt->format_context->streams[wgt->videoindex]->time_base.den;

//                    double base = av_q2d(wgt->format_context->streams[wgt->videoindex]->time_base);
//                    cout<<"decoder time:"<<wgt->pkt->dts <<"  "<<1000*wgt->pkt->dts*base<<endl;
//                    cout<<"video play time:"<<wgt->pkt->pts<<"  "<<1000*wgt->pkt->pts*base<<endl;
//                    cout<<"duration:"<<wgt->pkt->duration<<"   "<<1000*wgt->pkt->duration*base<<endl;

//                    wgt->format_context->duration

                    //rgb数据上锁

                    wgt->lock_pix.lock();

//                    cout<<"last  "<< wgt->video_buf_arr.size()<<endl;

//                    wgt->video_pts = wgt->pkt->pts;
                    Widget::video_buf vf;

//                    cout<<wgt->pkt->pts<<endl;
                    if(wgt->pkt->pts == LONG_LONG_MIN) //该帧的pts无效
                        vf.pts = wgt->pkt->dts;
                    else
                        vf.pts = wgt->pkt->pts;

                    //yuv转rgb
                    sws_scale(wgt->sw_context,
                            wgt->frame->data, wgt->frame->linesize,
                            0, wgt->codec_context->height,
                            wgt->out_buf, wgt->out_stride);

//                    cout<<"out_buf:"<<wgt->codec_context->height*wgt->codec_context->width*3<<endl;

                    try {
                        vf.out_buf = new uint8_t[wgt->codec_context->height*wgt->codec_context->width*3 + 1];
                    }
                    catch (bad_alloc &memExp) {
                        cout<<"分配失败"<<endl;
                        exit(-1);
                    }
                    memcpy(vf.out_buf, wgt->out_buf[0], wgt->codec_context->height*wgt->codec_context->width*3 + 1);

                    wgt->video_buf_arr.push_back(vf);

//                    cout<<"next: "<< wgt->video_buf_arr.size()<<endl;

                    wgt->lock_pix.unlock();


                    ///显示交给主线程定时器
                    stop = true;
                }
                else {perror("avcodec_receive_frame"); }
            }
            else if(wgt->pkt->stream_index==wgt->audioindex)
            {
                ret = avcodec_send_packet(wgt->au_code_context, wgt->pkt);
                ret = avcodec_receive_frame(wgt->au_code_context, wgt->frame);

                if(ret==0)
                {

//                    double base = av_q2d(wgt->format_context->streams[wgt->videoindex]->time_base);
//                    cout<<wgt->format_context->streams[wgt->videoindex]->time_base.den<<"video_base: "<<base<<endl;
//                    cout<<wgt->format_context->streams[wgt->audioindex]->time_base.den<<"audio_base: "<<base<<endl;
//                    cout<<"audio play time:"<<wgt->pkt->pts<<"  "<<1000*wgt->pkt->pts*base<<endl;
//                    cout<<"audio duration:"<<wgt->pkt->duration<<"   "<<1000*wgt->pkt->duration*base<<endl;

                    //音频数据上锁
                    wgt->lock_audio.lock();

                    struct Widget::audio_buf ad;

                    ad.bufsize = av_samples_get_buffer_size(nullptr, wgt->frame->ch_layout.nb_channels, wgt->frame->nb_samples,
                                                            AV_SAMPLE_FMT_S16, 0);
                    ad.read_buf = new uint8_t[ad.bufsize];
                    ad.pts = wgt->pkt->pts;

                    ret = swr_convert(wgt->swr_ctx, &ad.read_buf, wgt->frame->nb_samples, (const uint8_t**)(wgt->frame->data), wgt->frame->nb_samples);

//                    wgt->bufsize = av_samples_get_buffer_size(nullptr, wgt->frame->ch_layout.nb_channels, wgt->frame->nb_samples,
//                                                             AV_SAMPLE_FMT_S16, 0);
//                    wgt->read_buf = new uint8_t[wgt->bufsize];
//                    ret = swr_convert(wgt->swr_ctx, &wgt->read_buf, wgt->frame->nb_samples, (const uint8_t**)(wgt->frame->data), wgt->frame->nb_samples);

                    if(ret<0)
                    {
                        perror("swr_convert");
//                        delete[] wgt->read_buf;
                        delete[] ad.read_buf;
                        wgt->lock_audio.unlock();
                        return ;
                    }

                    wgt->audio_buf_arr.push_back(ad); //加入任务队列

                    wgt->lock_audio.unlock();

                    ///音频播放交给主线程定时器
//                    stop = true;
//                    qDebug()<<"转换完成"<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
//                    qDebug()<<"...";
                }


            }

            //解析一次停止
//            stop = true;
//            qDebug()<<"转换完成"<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        }

    }
}

void Read_frame::receive_info(Widget* wgt)
{
    this->wgt = wgt;
}

void Read_frame::receive_run()
{
    stop = false;
}

///////////////////////

void Widget::show_error(int err)
{
    char buf[1024];
    av_strerror(err, buf, 1024);
    qDebug()<<"error: "<<buf;
}

void Widget::init()
{
    //控件/////////////////////////////
    ui->lineEdit->setEnabled(false);
    ui->chose_file->setEnabled(true);
    ui->play->setEnabled(false);
    ui->pause->setEnabled(false);
    ui->stop->setEnabled(false);
    ui->close->setEnabled(true);
    ui->label->clear();

    //变量
    pls = CHOSE_FILE;  //状态转移
    fd = nullptr;
    parsed = 0;

    bool ok;
    speed = ui->speed->currentText().toDouble(&ok);
    if(!ok) cout<<"获取倍速失败！"<<endl;
    cout<<speed<<endl;

    //解码/////////////////////////////
    videoindex = -1;
    in_format = nullptr;
    dict = nullptr;

    format_context = nullptr;
    codec = nullptr;
    codec_context = nullptr;
    pkt = nullptr;
    frame = nullptr;
    sw_context = nullptr;
    for(int i=0;i<4;++i) out_buf[i] = nullptr;

    swr_ctx = nullptr;
    audioOutput = nullptr;
    audioDevice = nullptr;

//    read_buf = nullptr;

    read_t = new Read_frame(this);  //TODO: 自己加入this，然后后面又把this传给这里，会不会有问题
}

void Widget::free_decoder() //可能会重复进行
{
    ///音频///////
    if(swr_ctx) swr_free(&swr_ctx);  //这里会崩溃?? 加了初始化为null好了

//    cout<<12<<endl;

    while(!audio_buf_arr.empty())
    {
        delete [] audio_buf_arr.front().read_buf;
        audio_buf_arr.pop_front();
    }

//    cout<<10<<endl;

    //delete音乐设备
    if(audioOutput)
    {
        audioOutput->stop();
        delete audioOutput;
        audioOutput = nullptr;
    }

    if(au_code_context) avcodec_close(au_code_context);

//    cout<<11<<endl;
    /////////
    ///视频////
    while(!video_buf_arr.empty())
    {
        delete [] video_buf_arr.front().out_buf;
//        av_freep(&video_buf_arr.front().out_buf);
        video_buf_arr.pop_front();
    }

    av_freep(&out_buf[0]);
    if(sw_context) {sws_freeContext(sw_context); sw_context = nullptr;}
    if(frame) av_frame_free(&frame);
    if(pkt) av_packet_free(&pkt);
    if(codec_context) avcodec_close(codec_context);
    if(format_context) {avformat_close_input(&format_context);format_context = nullptr;} //
    if(dict) {av_dict_free(&dict); dict = nullptr;}
    in_format = nullptr;
//    if(in_format) {delete in_format; in_format = nullptr;}
}

bool Widget::init_decoder() //读完 重新开始 换文件 需要释放
{
    parsed = 0;

    int ret;

    format_context = avformat_alloc_context(); //TODO: 不加这句好像也没事  摄像头加上这句话还有问题  现在没问题了。。。

    ret = avformat_open_input(&format_context, file_name.c_str(), in_format, &dict);
    if(ret<0){ show_error(ret); return 0;}

    ret = avformat_find_stream_info(format_context, nullptr);
    if(ret<0){ show_error(ret);  return 0;}

    videoindex = -1;
    for(unsigned int i=0;i<format_context->nb_streams;++i)
        if(format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoindex = i;
            break;
        }

//    cout<<"id:"<<format_context->streams[videoindex]->codecpar->codec_id<<endl;

    codec = avcodec_find_decoder(format_context->streams[videoindex]->codecpar->codec_id);
    if(!codec){perror("avcodec_find_decoder"); return 0;}

    codec_context = avcodec_alloc_context3(codec);
    if(!codec_context){perror("avcodec_alloc_context3"); return 0;}

    //还需将信息拷过来？？
    avcodec_parameters_to_context(codec_context, format_context->streams[videoindex]->codecpar);

    ret = avcodec_open2(codec_context, codec, nullptr);
    if(ret<0){ show_error(ret); return 0;}

    pkt = av_packet_alloc();
    if(!pkt){perror("av_packet_alloc"); return 0;}

    frame = av_frame_alloc();
    if(!frame){perror("av_frame_alloc"); return 0;}

    ///音频///////////////////////////////////////////////////////
    audioindex = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audioindex < 0) { cout<<"未找到音频信号"<<endl; }  //视频流

    if(audioindex>=0)
    {
        au_coder = avcodec_find_decoder(format_context->streams[audioindex]->codecpar->codec_id);
        if(au_coder == nullptr) {cout<<"未找到对应类型的解码器!"<<endl; return 0;}

        au_code_context = avcodec_alloc_context3(au_coder);
        if(!au_code_context) {cout<<"avcodec_alloc_context3 failed!"<<endl; return 0;}

        ret = avcodec_parameters_to_context(au_code_context, format_context->streams[audioindex]->codecpar);
        if(ret<0) {show_error(ret); return 0;}

        ret = avcodec_open2(au_code_context, au_coder, nullptr);
        if(ret<0) {show_error(ret); return 0;}
    }



    ////////////////////////////////////////////////////////////////////


    //读出有效一帧  获取宽高
    int i=0;
    bool audio=(audioindex<0),video=false; //音视频是否读出一帧有效帧
    while(1)
    {
        ++i; //TODO:
        ret = av_read_frame(format_context, pkt);
        if(ret<0)
        {
            show_error(ret);  //-541478725 表示文件尾
            return 0;
        }
        if(pkt->stream_index==videoindex)
        {
            ret = avcodec_send_packet(codec_context, pkt);
            ret = avcodec_receive_frame(codec_context, frame);
            if(ret==0)
            {
                //计算帧率
                double base = av_q2d(format_context->streams[videoindex]->time_base);
//                cout<<"duration:"<<pkt->duration<<"   "<<1000*pkt->duration*base<<endl;
                pic_dur = 1000*pkt->duration*base;
                cout<<"pic_dur: "<<pic_dur<<endl;

                //获取色域转换上下文
                sw_context = sws_getContext(codec_context->width, codec_context->height,
                                            codec_context->pix_fmt,
                                            codec_context->width, codec_context->height,
                                            AV_PIX_FMT_RGB24, SWS_POINT,
                                            nullptr, nullptr, nullptr);
                if(!sw_context){perror("sws_getContext"); return 0;}

                //初始化rgb
                ret = av_image_alloc(out_buf, out_stride, codec_context->width, codec_context->height, AV_PIX_FMT_RGB24, 1);
                if(ret<0){ show_error(ret); return 0;}

                //yuv转rgb
                sws_scale(sw_context,
                        frame->data, frame->linesize,
                        0, codec_context->height,
                        out_buf, out_stride);

                QImage img(out_buf[0], codec_context->width, codec_context->height, QImage::Format_RGB888);
                QPixmap pix = QPixmap::fromImage(img);
                ui->label->resize(codec_context->width, codec_context->height);
                ui->label->setPixmap(pix);
                video = true;
                if(audio&&video) break;
            }
        }
        else if(pkt->stream_index==audioindex)  // 音频
        {
            ret = avcodec_send_packet(au_code_context, pkt);
            ret = avcodec_receive_frame(au_code_context, frame);
            if(ret==0)
            {
                //计算帧率
                double base = av_q2d(format_context->streams[audioindex]->time_base);
//                cout<<"duration:"<<pkt->duration<<"   "<<1000*pkt->duration*base<<endl;
                aud_dur = 1000*pkt->duration*base;
                cout<<"aud_dur: "<<aud_dur<<endl;

                /// 因为解码数据排列方式原因，音频不能直接播放，需要根据音频解码上下文设置
                /// 并初始化转换上下文
                swr_ctx = swr_alloc();
                ret = swr_alloc_set_opts2(&swr_ctx,
                            &au_code_context->ch_layout, AV_SAMPLE_FMT_S16, au_code_context->sample_rate,
                            &au_code_context->ch_layout, au_code_context->sample_fmt, au_code_context->sample_rate,
                            0, nullptr);
                if( ret<0 ) {show_error(ret); return 0;}
                swr_init(swr_ctx);      //TODO: 不知道啥用的

                int bufsize = av_samples_get_buffer_size(nullptr, frame->ch_layout.nb_channels, frame->nb_samples,
                                                         AV_SAMPLE_FMT_S16, 0);
                uint8_t *buf = new uint8_t[bufsize];
                swr_convert(swr_ctx, &buf, frame->nb_samples, (const uint8_t**)(frame->data), frame->nb_samples);

                audioFormat.setSampleRate(au_code_context->sample_rate);
                audioFormat.setChannelCount(au_code_context->ch_layout.nb_channels);
                audioFormat.setSampleSize(8*av_get_bytes_per_sample(AV_SAMPLE_FMT_S16));
                audioFormat.setSampleType(QAudioFormat::SignedInt);
                audioFormat.setCodec("audio/pcm");

                //还需要判断该格式设备是否支持

                audioOutput = new QAudioOutput(audioFormat, this);
                audioDevice = audioOutput->start();
                audioDevice->write((const char*)buf, bufsize);
                delete[] buf;

                audio = true;
                if(audio&&video) break;
            }

        }
    }
    return 1;
}


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    init();

    ///close
    connect(ui->close, &QPushButton::clicked, this, &QWidget::close);

    ///chose file
    connect(ui->chose_file, &QPushButton::clicked, [=]()mutable{  //这里如果用& filename会报错 和vs里不一样

        QString name = QFileDialog::getOpenFileName(this, "open file", "/", "Movies (*.mp4 *.avi *.mkv *.flv *.ts *.mov)");
        qDebug()<<name<<endl;
        file_name = name.toStdString();
        ui->lineEdit->setText(name);

        fclose(fd);
        fd = fopen(file_name.c_str(), "r");
        if(!fd)
        {
            perror("fopen");
            ui->label->clear();
            ui->label->setText(QString("当前文件打开错误！"));
            return;
        }

        ui->label->clear();
        ui->label->setText(QString("即将播放")+name);

        ui->play->setEnabled(true);
        ui->pause->setEnabled(false);
        ui->stop->setEnabled(false);

        ui->speed->setEnabled(true);  //

        pls = START;

        if(read_t->isRunning())  //不知道子线程是否运行  z中途播放
        {
            read_t->requestInterruption();
            read_t->quit();
            read_t->wait(1000);
        }

        free_decoder();

    });

    ///pause
    connect(ui->pause, &QPushButton::clicked, [&](){
        pls = PAUSE;

        ui->pause->setEnabled(false);
        ui->play->setEnabled(true);
        ui->stop->setEnabled(true);
    });

    ///stop
    connect(ui->stop, &QPushButton::clicked, [&](){
        pls = STOP;

        ui->stop->setEnabled(false);
        ui->pause->setEnabled(false);

        if(file_name.find("/") != -1)   //TODO: 表明是流数据  如果是流数据要清除视频缓冲
            ui->play->setEnabled(true); //需要重新点击摄像头，里面有必要操作 除非再抽象出来
        else
            ui->play->setEnabled(false);

//        timer_pic->stop();    //不能停止，需要通过定时器释放数据
//        if(audioindex>=0)
//        {
//            timer_aud->stop();
//        }


        ui->label->clear();
    });

    ///speed
    connect(ui->speed, &QComboBox::currentTextChanged, [&](){
        bool ok;
        speed = ui->speed->currentText().toDouble(&ok);
        if(!ok) cout<<"获取倍速失败！"<<endl;

        if(pls==CONTINUE)
        {
            timer_pic->start(int(pic_dur/speed));
            if(audioindex>=0)
            {
                timer_aud->start(int(aud_dur/speed));
                cout<<"speed: "<<int(pic_dur/speed)<<"  "<<int(aud_dur/speed)<<endl;
            }

        }

    });

    ///camera
    connect(ui->camera, &QPushButton::clicked, [&](){

        ///释放之前数据
        if(read_t->isRunning())  //不知道子线程是否运行  z中途播放
        {
            read_t->requestInterruption();
            read_t->quit();
            read_t->wait(1000);
        }

        free_decoder();
        ////////////////////

        avformat_network_init();
        avdevice_register_all();


        /// 获取可用摄像头列表  TODO:可能外接摄像头没有连接了但是还会被检测到缓存，需要在设备管理器禁用
        QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
        QString name;
        for(auto camera : cameras)
        {
            #if defined(Q_OS_WIN)
                name = "video=" +  camera.description(); // ffmpeg在Window下要使用video=description()
                if(name==QString("video=Chicony USB2.0 Camera")) break;  //我的摄像头名称
            #elif defined(Q_OS_LINUX)
                name = camera.description(); // ffmpeg在linux下要使用deviceName()
            #endif
            cout<<name.toStdString()<<endl;
        }

//        name = "video=Chicony USB2.0 Camera";  //应该是这个
//        name = "video=e2eSoft iVCam #1";  //应该是这个

        /// inputformat
        #if defined(Q_OS_WIN)
            in_format = av_find_input_format("dshow");            // Windows下如果没有则不能打开摄像头
        #elif defined(Q_OS_LINUX)
            in_format = av_find_input_format("video4linux2");       // Linux也可以不需要就可以打开摄像头
        #elif defined(Q_OS_MAC)
            in_format = av_find_input_format("avfoundation");
        #endif


        if(!in_format)
        {
            qWarning() << "查询AVInputFormat失败！";
            return ;
        }

        int ret;

        ret = av_dict_set(&dict, "input_format", "mjpeg", 0);
        if(ret<0)
        {
            cout<<"av_dict_set1 failed"<<endl;
            av_dict_free(&dict);
            dict = nullptr;
            return ;
        }
        ret = av_dict_set(&dict, "video_size", "640x480", 0);
        if(ret<0)
        {
            cout<<"av_dict_set2 failed"<<endl;
            av_dict_free(&dict);
            dict = nullptr;
            return ;
        }

        /// 设置控件
        qDebug()<<name<<endl;
        file_name = name.toStdString();
        ui->lineEdit->setText(name);


        ui->label->clear();
        ui->label->setText(QString("即将显示摄像头")+name);

        ui->play->setEnabled(true);
        ui->pause->setEnabled(false);
        ui->stop->setEnabled(false);

        ui->speed->setEnabled(false);  //TODO: 其他的未激活
        speed = 1.0;
        ui->speed->setCurrentIndex(2);

        pls = START;

        /// 退出之前子线程
        if(read_t->isRunning())  //不知道子线程是否运行  z中途播放
        {
            read_t->requestInterruption();
            read_t->quit();
            read_t->wait(1000);
        }
    });


    ///与解码通信
    //发送解码信息给 解码类
    connect(this, &Widget::you_can_start, read_t, &Read_frame::receive_info);
    //通知子线程继续运行
    connect(this, &Widget::can_continue, read_t, &Read_frame::receive_run);
    //接收状态量
    connect(read_t, &Read_frame::status, [&](bool parsed){
        this->parsed = parsed;
    });

    ///play
    connect(ui->play, &QPushButton::clicked, [&](){

        switch(pls){

        case STOP:
        case END:   // TODO: 如果end的时候还没有释放上一次的资源怎么办
        case START:
            if(!init_decoder()) //解码出错
            {
                pls = CHOSE_FILE;       //TODO: 直接变成这个会不会有bug
                ui->play->setEnabled(false);
                ui->pause->setEnabled(false);
                ui->stop->setEnabled(false);
                return ;
            }

            emit you_can_start(this);
            emit can_continue();
            read_t->start();  //解码子线程启动

            break;

        case PAUSE:
            break;

        case CONTINUE:
            break;

        default:
            break;
        }

        timer_pic->start(int(pic_dur/speed));
        if(audioindex>=0)
        {
            timer_aud->start(int(aud_dur/speed));
            cout<<"t1:"<<pic_dur<<"  "<<aud_dur<<endl;
            cout<<"speed: "<<int(pic_dur/speed)<<"  "<<int(aud_dur/speed)<<endl;
        }


        pls = CONTINUE;
        ui->play->setEnabled(false);
        ui->pause->setEnabled(true);
        ui->stop->setEnabled(true);

    });

    ///定时器
    timer_pic = new QTimer(this);
    timer_pic->setTimerType(Qt::PreciseTimer);
    connect(timer_pic, &QTimer::timeout, [&](){  //定时读出显示
        switch(pls){
        case CONTINUE:
            video_play();
            break;

        case PAUSE:
            if(file_name.find("/") == -1) //TODO: 表明是流数据  如果是流数据要清除视频缓冲
            {
                video_play();
            }
            break;

        case STOP:  //停止和播放结束 动作应该是一样的把
        case END:

            if(read_t->isRunning())
            {
                read_t->requestInterruption();
                read_t->quit();
                read_t->wait(1000); //等待释放资源 //当子线程正锁住的时候 但是也不会锁住啊
            }

            free_decoder();

            if(file_name.find("/") != -1) //TODO: 表明是流数据  如果是流数据要清除视频缓冲
                ui->play->setEnabled(true);  //需要重新点击摄像头，里面有必要操作 除非再抽象出来
            else
                ui->play->setEnabled(false);

            ui->pause->setEnabled(false);
            ui->stop->setEnabled(false);
            break;

        default:
            break;
        }
    });
//    timer_pic->start(41);  //24


    timer_aud = new QTimer(this);
    timer_aud->setTimerType(Qt::PreciseTimer);
    connect(timer_aud, &QTimer::timeout, [&](){  //定时读出播放
//        qDebug()<<"进入定时器"<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        switch(pls){
        case CONTINUE:
            audio_play();
            break;

//        case STOP:  //停止和播放结束 动作应该是一样的把   全由视频线程处理
//        case END:

//            if(read_t->isRunning())
//            {
//                read_t->requestInterruption();
//                read_t->quit();
//                read_t->wait(1000); //等待释放资源
//            }

//            free_decoder();
//            ui->play->setEnabled(true);
//            ui->pause->setEnabled(false);
//            ui->stop->setEnabled(false);
//            break;

        default:
            break;
        }
    });
//    timer_aud->start(24);  //24  TODO:
}

void Widget::video_play()
{

    lock_pix.lock();

    if(video_buf_arr.empty())
    {
        if(!parsed)
            emit can_continue();
        else
            pls = END;

        lock_pix.unlock();

        return ;
    }

    //之前这里使用变量接收的话会有问题，难道是弹出的时候自动释放对取数据？？也不对啊。。。
    video_pts = video_buf_arr.front().pts;

    if(pls!=PAUSE||file_name.find("/") != -1) //TODO: 表明是流数据 且处于暂停 不显示
    {
        QImage img(video_buf_arr.front().out_buf, codec_context->width, codec_context->height, QImage::Format_RGB888);
        QPixmap pix = QPixmap::fromImage(img);
        ui->label->resize(codec_context->width, codec_context->height);
        ui->label->setPixmap(pix);
    }

    delete [] video_buf_arr.front().out_buf;  //释放堆区数据 用av_freep会报错

    video_buf_arr.pop_front();

    lock_pix.unlock();


    if(pls == BAD)  //目前未使用
    {
        if(read_t->isRunning())
        {
            read_t->requestInterruption();
            read_t->quit();
            read_t->wait(1000); //等待释放资源
        }

        cout<<endl<<"视频解析出错！请重新选择..."<<endl<<endl;
        free_decoder();
        ui->play->setEnabled(false);
        ui->pause->setEnabled(false);
        ui->stop->setEnabled(false);
        pls = CHOSE_FILE;
    }

    emit can_continue();

}

void Widget::audio_play()
{
    //TODO:这样有没有可能 音频堆了很多了而视频还没放呢 暂时没考虑视频pts有问题 且未锁住视频的pts
//    if  当前 队列首 音频时间戳>视频时间戳 则返回

    //直到与当前视频帧时间戳之差在20ms之内播放
    double base = 1000*av_q2d(format_context->streams[videoindex]->time_base);
    if(audio_buf_arr.empty()||base*(audio_buf_arr.front().pts - video_pts)>100)
        return ;
    else if(base*(audio_buf_arr.front().pts - video_pts)<-100)  //若音频时间戳更慢了
    {
        while(!audio_buf_arr.empty()&&base*(audio_buf_arr.front().pts - video_pts)<-100)
        {
            delete [] audio_buf_arr.front().read_buf;
            audio_buf_arr.pop_front();
        }

        if(audio_buf_arr.empty())   //尝试跳到将来帧进行解码 这样图像应该会被干扰吧
        {
            emit can_continue();
            return;
        }
    }

    lock_audio.lock();

    audioDevice->write((const char*)audio_buf_arr.front().read_buf, audio_buf_arr.front().bufsize);
    delete[] audio_buf_arr.front().read_buf;
    audio_buf_arr.front().read_buf = nullptr;
    audio_buf_arr.pop_front();


    lock_audio.unlock();

    if(pls == BAD)
    {
        if(read_t->isRunning())
        {
            read_t->requestInterruption();
            read_t->quit();
            read_t->wait(1000); //等待释放资源
        }

        cout<<endl<<"视频解析出错！请重新选择..."<<endl<<endl;
        free_decoder();
        ui->play->setEnabled(false);
        ui->pause->setEnabled(false);
        ui->stop->setEnabled(false);
        pls = CHOSE_FILE;
    }

}

Widget::~Widget()
{

    if(read_t->isRunning())
    {
        read_t->requestInterruption();
        read_t->quit();
        read_t->wait(1000); //等待释放资源 //当子线程正锁住的时候 但是也不会锁住啊
    }

    free_decoder();

    if(fd)
    {
        fclose(fd);
        fd = nullptr;
    }

    delete ui;


}

