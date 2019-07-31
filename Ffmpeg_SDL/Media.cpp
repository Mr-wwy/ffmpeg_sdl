#include "Media.h"
#include "DisplayMediaTimer.h"
#include <QMutexLocker>

extern "C"{
#include <libavutil/time.h>
}
#include <QDebug>
#include <unistd.h>

static bool isExit = false;
const static long long  MAX_AUDIOQ_SIZE = (5 * 16 * 1024);//最大音频包大小（队列中所有加起来的大小）
const static long long   MAX_VIDEOQ_SIZE = (5 * 256 * 1024);//最大视频包大小（队列中所有视频包加起来的大小）

//时间基准计算
static double r2d(AVRational r)
{
    return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

Media::Media()
{
    av_register_all();//注册ffmpeg所有组件
    avformat_network_init();//注册网络组件
//    pFormatCtx = NULL;
    pFormatCtx = avformat_alloc_context();
    audio = new Audio;
    video = new Video;
}

//配置视频
Media *  Media::config()
{
    close();
//    QMutexLocker locker(&mutex);
    char errorbuf[1024] = { 0 };
    if(!pFormatCtx)
        pFormatCtx = avformat_alloc_context();
    AVDictionary* opts = NULL;
    av_dict_set(&opts, "stimeout", "20000000", 0);// 该函数是微秒 意思是10秒后没拉取到流就代表超时
//    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    av_dict_set(&opts, "buffer_size", "1024000", 0);
    int ret = avformat_open_input(&pFormatCtx, filename, NULL, &opts);//打开多媒体数据并且获得信息
    if(ret < 0){
        av_strerror(ret, errorbuf, sizeof(errorbuf));
        qDebug() << filename << ":" << errorbuf;
        return NULL;
    }
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0)//读取视音频数据并且获得信息
    {
        av_strerror(ret, errorbuf, sizeof(errorbuf));
        qDebug() << filename << ":" << errorbuf;
        return NULL;
    }

    //av_dump_format(pFormatCtx, 0, filename, 0);
    totalMs = ((pFormatCtx->duration / AV_TIME_BASE) * 1000);//计算视频总时长
    video->setStreamIndex(-1);//设置音视频流下标为-1，因为需要多次打开不同音视频
    audio->setStreamIndex(-1);
    for(uint32_t i = 0; i < pFormatCtx->nb_streams; i++)
    {   //循环查找视频中包含的流信息，直到找到音视频类型的流
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audio->getStreamIndex() < 0)
            audio->setStreamIndex(i);   //音频流

        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && video->getStreamIndex() < 0)
            video->setStreamIndex(i);   //视频流
    }

    if(audio->getStreamIndex() < 0 || video->getStreamIndex() < 0) //视音频流都有才继续
        return NULL;

    // 查找音频解码器
    AVCodec *pCodec = avcodec_find_decoder(pFormatCtx->streams[audio->getStreamIndex()]->codec->codec_id);
    if(!pCodec)
        return NULL;

    //设置音频流
    audio->setStream(pFormatCtx->streams[audio->getStreamIndex()]);

    audio->setAVCodecContext(avcodec_alloc_context3(pCodec));   //音频解码器上下文空间申请

    if(avcodec_copy_context(audio->getAVCodecContext(), pFormatCtx->streams[audio->getStreamIndex()]->codec) != 0)
        return NULL;

//    audio->getAVCodecContext()->coded_side_data = NULL;
//    audio->getAVCodecContext()->nb_coded_side_data = 0;

    //打开音频解码器并初始化上下文
    avcodec_open2(audio->getAVCodecContext(), pCodec, NULL);

    // 查找视频解码器
    AVCodec *pVCodec = avcodec_find_decoder(pFormatCtx->streams[video->getStreamIndex()]->codec->codec_id);
    if(!pVCodec)
        return NULL;

    // 设置视频流
    video->setVideoStream(pFormatCtx->streams[video->getStreamIndex()]);

    video->setAVCodecCotext(avcodec_alloc_context3(pVCodec));
    if(avcodec_copy_context(video->getAVCodecCotext(), pFormatCtx->streams[video->getStreamIndex()]->codec) != 0)
        return NULL;

    //打开视频解码器并初始化上下文
    avcodec_open2(video->getAVCodecCotext(), pVCodec, NULL);

    video->setFrameTimer(static_cast<double>(av_gettime()) / 1000000.0);//设置初始视频帧时间用于音视同步
    video->setFrameLastDelay(40e-3) ;//计算时间，TODO 设置上一帧的延时
    audio->audioPlay();//播放音频
    isPlay = true;
    DisplayMediaTimer::getInstance()->setPlay(isPlay);//音视频同步
    return this;
}

//设置视频文件
Media * Media::setMediaFile(const char * filename)
{
    this->filename = filename;
    return this;
}

//检查视频大小是否合法
bool Media::checkMediaSizeValid()
{
    if(this->audio == NULL || this->video == NULL)
        return true;

    Uint32 audioSize = this->audio->getAudioQueueSize();
    Uint32 videoSize = this->video->getVideoQueueSize();
    return (audioSize> MAX_AUDIOQ_SIZE || videoSize> MAX_VIDEOQ_SIZE);
}

//音频开始播放
void Media::startAudioPlay()
{
    audio->audioPlay();
}

//获取音视频文件格式上下文
AVFormatContext * Media::getAVFormatContext()
{
    return pFormatCtx;
}

//获取视频播放状态
bool Media::getIsPlaying()
{
    return isPlay;
}

//接收跳转的位置，跳转标识设置true
void Media::receivePos(float pos)
{
    currentPos = pos;
    isSeek = true;
}

//释放资源
void Media::close()
{
    totalMs = 0;
    isPlay = false;
    DisplayMediaTimer::getInstance()->setPlay(isPlay);
    video->setVideoEndflag(isPlay);
    if(video->swsContext)
    {
        sws_freeContext(video->swsContext);
        video->swsContext = NULL;
    }
    audio->audioClose();
    audio->clearPacket();
    video->clearFrames();
    video->clearPackets();
    video->close();
    if(pFormatCtx)
        avformat_close_input(&pFormatCtx);
}

Media::~Media()
{
    SDL_CloseAudio();
    isExit = true;
    wait();
    video->setVideostartflag(true);
    close();

    if(audio != NULL)
        delete audio;

    if(video != NULL)
        delete video;
}

//读取音视频包的线程处理函数
void Media::run()
{
    AVPacket packet;
    while(!isExit)
    {
        QMutexLocker locker(&mutex);
        if(!isPlay){//还没开始播放
            locker.unlock();
            msleep(100);
            continue;
        }
        if(audio == NULL || video == NULL){
            locker.unlock();
            break;
        }

        if(isSeek){//是否跳转视频的标识，在解压音视频读包时进行跳转
            int64_t stamp = 0;
            stamp = currentPos * video->getVideoStream()->duration;

            int ret = av_seek_frame(pFormatCtx, video->getStreamIndex(),
                stamp, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
            audio->clearPacket();//要清空所有队列
            video->clearPackets();
            video->clearFrames();
            isSeek = false;
        }

        bool isInvalid = checkMediaSizeValid();//音视队列超过一定量时先不读包，等待包消费
        if(isInvalid){
            locker.unlock();
            msleep(10);
            continue;
        }
        if(!pFormatCtx){
            locker.unlock();
            msleep(10);
            continue;
        }
        int ret = av_read_frame(pFormatCtx, &packet);//读帧音视频包
        if(ret < 0)
        {
            if(ret == AVERROR_EOF){//读包出错
                locker.unlock();
                if(video->end_flag)
                {
                    //播放结束
                    totalMs = 0;
                    isPlay = false;
                    DisplayMediaTimer::getInstance()->setPlay(isPlay);
                    SDL_PauseAudio(1); //退出Callback
                    break;
                }
                continue;
            }

            if(pFormatCtx->pb->error == 0) // 没有错误就等待下次读
            {
                locker.unlock();
                msleep(100);
                continue;
            }
        }

        if(audio!=NULL && packet.stream_index == audio->getStreamIndex()) // 音频包队列此处入队
        {
//            locker.unlock();
            audio->enqueuePacket(packet);
        }

        else if(video != NULL && packet.stream_index == video->getStreamIndex()) // 视频包队列此处入队
        {
//            locker.unlock();
            video->enqueuePacket(packet);
        }
        else
            av_packet_unref(&packet);
        locker.unlock();
    }
    if(packet.size>=0)
        av_packet_unref(&packet);//包没数据不能释放
}

