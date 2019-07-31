#include "PacketQueue.h"
#include <iostream>
#include <QMutexLocker>
static bool isExit = false;
PacketQueue::PacketQueue()
{
    size = 0;
}

PacketQueue::~PacketQueue()
{
    QMutexLocker locker(&mutex);
    isExit = true;
    locker.unlock();
}

//包队列入队
bool PacketQueue::enQueue(const AVPacket packet)
{
    QMutexLocker locker(&mutex);
    queue.push(packet);
    size += packet.size;
    cond.wakeOne();
    locker.unlock();
    return true;
}

//包队列出队
AVPacket PacketQueue::deQueue()
{
    bool ret = false;
    AVPacket pkt;
    QMutexLocker locker(&mutex);
    while(true)
    {
        if(!queue.empty())
        {
            pkt = queue.front();
            queue.pop();
            size -= pkt.size;
            ret = true;
            break;
        }
        else
            cond.wait(&mutex);
    }
    locker.unlock();
    return pkt;
}

//获取包大小
Uint32 PacketQueue::getPacketSize()
{
//    QMutexLocker locker(&mutex);
    return size;
}

//清空队列
void PacketQueue::queueFlush()
{
     while(!queue.empty())
     {
         AVPacket pkt = deQueue();
     }
}
