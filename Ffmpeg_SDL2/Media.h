#ifndef MEDIA_H
#define MEDIA_H

#include <QThread>
#include "Audio.h"
#include "Video.h"
#include <QMutex>

#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

#ifdef __cplusplus
extern "C"{
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
}
#endif

class Media : public QThread
{
    Q_OBJECT
public:
    static Media *getInstance(){
        static Media media;
        return &media;
    }
    ~Media();
    void run();
    bool getIsPlaying();
    Media * config();
    Media * setMediaFile(const char*filename);
    bool checkMediaSizeValid();
    void startAudioPlay();
    AVFormatContext *getAVFormatContext();
    Video *video;
    Audio *audio;
    void close();
    int totalMs = 0;
    int pts = 0;
    float currentPos = 0;
    bool isSeek = false;
private:
    AVFormatContext *pFormatCtx;
    Media();
    const char *filename;
    QMutex mutex;
    bool isPlay;

public slots:
    void receivePos(float pos);

};

#endif // MEDIA_H
