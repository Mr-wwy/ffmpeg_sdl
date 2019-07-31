#include "FrameQueue.h"
#include <QMutexLocker>
#include <QDebug>
FrameQueue::FrameQueue()
{
}

//帧队列入队
bool FrameQueue::enQueue(const AVFrame* frame)
{
    AVFrame* p = av_frame_alloc();

    int ret = av_frame_ref(p, frame);
    if(ret < 0)
        return false;
    p->opaque = (void *)new double(*(double*)p->opaque); //上一个指向的是一个局部的变量，这里重新分配pts空间
    QMutexLocker locker(&mutex);
    queue.push(p);  //队尾插入一个元素
    cond.wakeOne();
    locker.unlock();
    end_flag = true;
    return true;
}

//帧队列
AVFrame * FrameQueue::deQueue()
{
    AVFrame *tmp;

    if(!queue.empty())
    {
        tmp = queue.front(); //取出队列中最前面的元素
        queue.pop();   //将队列中最前面的元素删除
    }
    return tmp;
}

//获取队列大小
int FrameQueue::getQueueSize()
{
    return queue.size(); //元素的个数
}

void FrameQueue::setEndflag(bool flag)
{
    end_flag = flag;
}

//清空帧队列
void FrameQueue::queueFlush() {
    while(!queue.empty())
    {
        AVFrame *frame = deQueue();
        av_frame_unref(frame);
        av_frame_free(&frame);
    }
}
