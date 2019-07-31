#include "ImageFilter.h"
#include<QThread>
#include <QMutexLocker>
#include "QImageHandler.h"
#include<QDebug>

//过滤图片操作
QImage * ImageFilter::filter(QImage *& img)
{
    QMutexLocker locker(&mutex);
    QImageHandler handler(img);
    for(int i = 0; i < tasks.size(); i++)
    {
        switch(tasks[i].type)
        {
            case XTASK_MIRRORLEFTANDRIGHT://左右镜像
                handler.mirrorLeftAndRight();
                break;
            case XTASK_MIRRORUPANDDOWN://上下镜像
                handler.mirrorUpAndDown();
                break;
            default:
                break;
        }
    }
    locker.unlock();
    return handler.getHandleImage();//返回操作后的图片
}

//增加操作任务
void ImageFilter::addTask(XTask task)
{
    QMutexLocker locker(&mutex);
    tasks.push_back(task);
    locker.unlock();
}

//增加颜色任务
void ImageFilter::addColorTask(ColorTask task)
{
    QMutexLocker locker(&mutex);
    colorTasks.push_back(task);
    locker.unlock();
}

//过滤颜色
QImage * ImageFilter::filterColor(QImage *& img)
{
    QMutexLocker locker(&mutex);
    QImageHandler handler(img);
    for(int i = 0; i < colorTasks.size(); i++)
    {
        switch(colorTasks[i].type)
        {
            case COLRTASK_GRAY_TO_RGBA://灰度图转RGB图
                handler.gray2RGB();
                break;
            case COLRTASK_RGBA_TO_GRAY://RGB图转灰度图
                handler.rgb2Gray();
                break;
            default:
                break;
        }
    }
    colorTasks.clear();
    locker.unlock();
    return handler.getHandleImage();
}

//清空任务
void ImageFilter::clear()
{
    QMutexLocker locker(&mutex);
    tasks.clear();
    colorTasks.clear();
    locker.unlock();
}

ImageFilter::ImageFilter()
{
}

ImageFilter::~ImageFilter()
{
    clear();
}
