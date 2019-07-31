#include "QImageHandler.h"

#include <QMatrix>

QImageHandler::QImageHandler(QImage *& srcImage)
{
    this->desImage = srcImage;
}


QImageHandler::~QImageHandler()
{
}

//视频上下镜像操作
void QImageHandler::mirrorUpAndDown()
{
    *desImage = desImage->mirrored(false,true);
}

//视屏左右镜像操作
void QImageHandler::mirrorLeftAndRight()
{
    *desImage = desImage->mirrored(true, false);
}

//rgb转灰度图
void QImageHandler::rgb2Gray()
{
    int imageWidth = 0;
    int imageHeight = 0;
    if(desImage && desImage->format() != QImage::Format_Grayscale8){
        imageWidth = desImage->width();
        imageHeight = desImage->height();
        if(desImage){
            delete desImage;
            desImage = NULL;
        }
        desImage = new QImage(imageWidth, imageHeight, QImage::Format_Grayscale8);
    }
}

//灰度图转rgb
void QImageHandler::gray2RGB()
{
    int imageWidth = 0;
    int imageHeight = 0;
    if(desImage && desImage->format() != QImage::Format_ARGB32){
        imageWidth = desImage->width();
        imageHeight = desImage->height();
        if(desImage){
            delete desImage;
            desImage = NULL;
        }

        desImage = new QImage(imageWidth, imageHeight, QImage::Format_ARGB32);
    }
}
