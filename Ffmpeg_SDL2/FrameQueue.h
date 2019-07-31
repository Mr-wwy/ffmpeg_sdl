#ifndef FRAMEQUEUE_H
#define FRAMEQUEUE_H

#include <queue>
#include <QMutex>
#include <QWaitCondition>
extern "C"{

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class FrameQueue
{
public:
    static const int capacity = 30;
    FrameQueue();
    bool enQueue(const AVFrame* frame);
    AVFrame * deQueue();
    int getQueueSize();
    void queueFlush();
    void setEndflag(bool flag);

    bool end_flag = false;  //视频结束判断

private:
    std::queue<AVFrame*> queue;
    QMutex mutex;
    QWaitCondition cond;


};

#endif // FRAMEQUEUE_H
