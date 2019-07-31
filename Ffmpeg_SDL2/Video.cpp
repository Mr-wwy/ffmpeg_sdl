#include "Video.h"
#include <QMutexLocker>
#include <unistd.h>
#include <QDebug>

static bool isExit = false;

Video::Video()
{
    frameTimer = 0.0;
    frameLastDelay = 0.0;
    frameLastPts = 0.0;
    videoClock = 0.0;
    videoContext = NULL;
    videoPackets = new PacketQueue;
}


Video::~Video()
{
//    QMutexLocker locker(&mutex);
    delete videoPackets;
    isExit = true;
//    locker.unlock();
    wait();

}

void Video::close()
{
    if(videoContext)
    {
        sleep(1);
        avcodec_close(videoContext);
        videoContext = NULL;
    }
}

//获取流下标
int Video::getStreamIndex()
{
    return streamIndex;
}

//设置流下标
void Video::setStreamIndex(const int &streamIndex)
{
    this->streamIndex = streamIndex;
}

//获取视频队列大小
int Video::getVideoQueueSize()
{
    return videoPackets->getPacketSize();
}

//包入队
void Video::enqueuePacket(const AVPacket &pkt)
{
    videoPackets->enQueue(pkt);
}

//帧队列出队
AVFrame * Video::dequeueFrame()
{
    return frameQueue.deQueue();
}

//计算同步视频的播放时间
double Video::synchronizeVideo(AVFrame *&srcFrame, double &pts)
{
    double frameDelay;
    if(pts != 0)
        videoClock = pts; // Get pts,then set video clock to it
    else
        pts = videoClock; // Don't get pts,set it to video clock
    frameDelay = av_q2d(stream->codec->time_base);
    frameDelay += srcFrame->repeat_pict * (frameDelay * 0.5);
    videoClock += frameDelay;
    return pts;
}

//获取视频流
AVStream * Video::getVideoStream()
{
    return stream;
}

//设置视频流
void Video::setVideoStream(AVStream *& videoStream)
{
    this->stream = videoStream;
}

//获取视频解码器上下文
AVCodecContext * Video::getAVCodecCotext()
{
    return this->videoContext;
}

//设置视频解码器上下文
void Video::setAVCodecCotext(AVCodecContext * avCodecContext)
{
    this->videoContext = avCodecContext;
}

//设置帧时间
void Video::setFrameTimer(const double & frameTimer)
{
    this->frameTimer = frameTimer;
}

void Video::setVideostartflag(bool flag)
{
    isExit = flag;
    msleep(800);
}

//视频读帧线程处理函数
void Video::run()
{
    AVFrame * frame = av_frame_alloc();
    double pts;
    AVPacket pkt;
    while(!isExit)
    {
        QMutexLocker locker(&mutex);
        if(frameQueue.getQueueSize() == 0 && frameQueue.end_flag)
        {   //视频结束判断
            frameQueue.setEndflag(false);
            end_flag = true;
            continue;
        }
        if(frameQueue.getQueueSize() >= FrameQueue::capacity) {//视频帧多于30帧就等待消费
            locker.unlock();
            msleep(100);
            continue;
        }
        if(videoPackets->getPacketSize() == 0) {//没包等待包入队
            locker.unlock();
            msleep(100);
            continue;
        }
        pkt = videoPackets->deQueue();//取包

        int ret = avcodec_send_packet(videoContext, &pkt); //视频包数据输出给解码器上下文
        if(ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            locker.unlock();
            continue;
        }
        av_frame_unref(frame); //释放AVFrame
        ret = avcodec_receive_frame(videoContext, frame); //从解码器上下文中获取解码的输出数据帧
        if(ret < 0 && ret != AVERROR_EOF) {
            locker.unlock();
            continue;
        }
        //获取解码的视频帧时间
        if((pts = av_frame_get_best_effort_timestamp(frame)) == AV_NOPTS_VALUE)
            pts = 0;
        pts *= av_q2d(stream->time_base);
        pts = synchronizeVideo(frame, pts);//同步视频播放时间
        frame->opaque = &pts;
        frameQueue.enQueue(frame);//帧入队
        locker.unlock();
    }
    av_frame_free(&frame);
}

//获取帧时间
double Video::getFrameTimer()
{
    return frameTimer;
}

//设置上一帧的播放时间
void Video::setFrameLastPts(const double & frameLastPts)
{
    this->frameLastPts = frameLastPts;
}

//获取上一帧的播放时间
double Video::getFrameLastPts()
{
    return frameLastPts;
}

//设置上一帧的延时
void Video::setFrameLastDelay(const double & frameLastDelay)
{
    this->frameLastDelay = frameLastDelay;
}

//获取上一帧延时
double Video::getFrameLastDelay()
{
    return frameLastDelay;
}

//设置视频时钟
void Video::setVideoClock(const double & videoClock)
{
    this->videoClock = videoClock;
}

//获取视频时钟
double Video::getVideoClock()
{
    return videoClock;
}

//获取帧大小
int Video::getVideoFrameSiez()
{
    return frameQueue.getQueueSize();
}

//清空帧队列
void Video::clearFrames()
{
    frameQueue.queueFlush();
}

//清空包队列
void Video::clearPackets()
{
    videoPackets->queueFlush();
}

void Video::setVideoEndflag(bool flag)
{
    end_flag = flag;
}

