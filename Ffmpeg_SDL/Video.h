#ifndef VIDEO_H
#define VIDEO_H

#include "PacketQueue.h"
#include "FrameQueue.h"
#include <QThread>
extern "C"{

#include <libavformat/avformat.h>
#include<libswscale/swscale.h>
#include <libswresample/swresample.h>
}

class Video:public QThread
{
public:
    Video();
    ~Video();
    void run();
    double synchronizeVideo(AVFrame *&srcFrame, double &pts);
    int getStreamIndex();
    AVCodecContext * getAVCodecCotext();
    void enqueuePacket(const AVPacket &pkt);

    AVFrame * dequeueFrame();
    void setStreamIndex(const int &streamIndex);
    int getVideoQueueSize();
    void setVideoStream(AVStream *& stream);
    AVStream * getVideoStream();
    void setAVCodecCotext(AVCodecContext *avCodecContext);
    void setFrameTimer(const double &frameTimer);
    double getFrameTimer();
    void setFrameLastPts(const double &frameLastPts);
    double getFrameLastPts();
    void setFrameLastDelay(const double &frameLastDelay);
    double getFrameLastDelay();
    void setVideoClock(const double &videoClock);
    double getVideoClock();
    int getVideoFrameSiez();
    void setVideostartflag(bool flag);
    void setVideoEndflag(bool flag);
    void clearFrames();
    void clearPackets();
    void close();

    bool end_flag = false;  //视频结束判断
    SwsContext *swsContext = NULL;  //(图像转码器)
private:
    double frameTimer;         // Sync fields
    double frameLastPts;
    double frameLastDelay;
    double videoClock;
    PacketQueue *videoPackets;
    FrameQueue frameQueue;
    AVStream *stream;   //视频流
    int streamIndex = -1;
    QMutex mutex;
    AVCodecContext *videoContext;   //视频解码器上下文环境
};

#endif // VIDEO_H
