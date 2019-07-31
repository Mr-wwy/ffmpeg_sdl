#include "Audio.h"
#include "Media.h"
#include <fstream>
#include <QDebug>

extern "C"{
#include <libswresample/swresample.h>
#include<SDL.h>
}
static int audioVolume = 64;
Audio::Audio()
{
    audioContext = NULL;
    streamIndex = -1;
    stream = NULL;
    audioClock = 0;
    audioBuff = new uint8_t[BUFFERSIZE];
//    audioBuff = NULL;
    audioBuffSize = 0;
    audioBuffIndex = 0;

//    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
//        qDebug() << SDL_GetError();
}

//计算时间基准
static double r2d(AVRational r)
{
    return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

Audio::~Audio()
{
    SDL_Quit();
    if(audioBuff)
        delete[] audioBuff;
}

//关闭音频,释放资源
bool Audio::audioClose()
{
    SDL_PauseAudio(1); //退出Callback(),暂停声音
    SDL_CloseAudio();  //关闭SDL
    if(audioContext)
    {
        avcodec_close(audioContext);
        audioContext = NULL;
    }
    if(audioBuff)
    {
        delete[] audioBuff;
        audioBuff = NULL;
    }
    audioBuff = new uint8_t[BUFFERSIZE];
    return true;
}

//播放音频
bool Audio::audioPlay()
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
            qDebug() << SDL_GetError();
            return -1;
    }

    SDL_AudioSpec desired;

    desired.freq = audioContext->sample_rate;   //1秒采集声音的个数
    desired.channels = audioContext->channels; //声道数: 1 单声道, 2 立体声
    desired.format = AUDIO_S16SYS;  //音频数据格式
    desired.samples = 1024;
    desired.silence = 0;
    desired.userdata = this;    //传递给回调的指针
    desired.callback = audioCallback;
    qDebug() << "audio play1";
    if(SDL_OpenAudio(&desired, NULL) < 0)
    {
        qDebug() << "openAudio fail!" << " " << SDL_GetError();
        return false;
    }

    qDebug() << "audio play";
    SDL_PauseAudio(0); // 进入Callback(),播放声音

    return true;
}

//获取当前音频时钟
double Audio::getCurrentAudioClock()
{
    int hw_buf_size = audioBuffSize - audioBuffIndex;
    int bytes_per_sec = stream->codec->sample_rate * audioContext->channels * 2;
    double pts = audioClock - static_cast<double>(hw_buf_size) / bytes_per_sec;
    return pts;
}

//获取流下标
int Audio::getStreamIndex()
{
    return streamIndex;
}

//设置流下标
void Audio::setStreamIndex(const int streamIndex)
{
    this->streamIndex = streamIndex;
}

//获取音频队列大小
int Audio::getAudioQueueSize()
{
    return audiaPackets.getPacketSize();
}

//音频包入队
void Audio::enqueuePacket(const AVPacket pkt)
{
    audiaPackets.enQueue(pkt);
}

//音频出队
AVPacket Audio::dequeuePacket()
{
    return audiaPackets.deQueue();
}

//获取音频缓冲
uint8_t* Audio::getAudioBuff()
{
    return audioBuff;
}

//设置音频缓冲
void Audio::setAudioBuff(uint8_t*&  audioBuff)
{
    this->audioBuff = audioBuff;
}

//获取音频缓冲大小
uint32_t Audio::getAudioBuffSize()
{
    return audioBuffSize;
}

//设置缓冲大小
void Audio::setAudioBuffSize(uint32_t audioBuffSize)
{
    this->audioBuffSize = audioBuffSize;
}

//获取音频缓冲下标
uint32_t Audio::getAudioBuffIndex()
{
    return audioBuffIndex;
}

//设置音频缓冲的下标
void Audio::setAudioBuffIndex(uint32_t audioBuffIndex)
{
    this->audioBuffIndex = audioBuffIndex;
}

//获取音频时钟
double Audio::getAudioClock()
{
    return audioClock;
}

//设置音频时钟
void Audio::setAudioClock(const double & audioClock)
{
    this->audioClock = audioClock;
}

//获取音频流
AVStream * Audio::getStream()
{
    return this->stream;
}

//设置音频流
void Audio::setStream(AVStream *& stream)
{
    this->stream = stream;
}

//获取解码器上下文
AVCodecContext * Audio::getAVCodecContext()
{
    return this->audioContext;
}

//设置音频解码器上下文
void Audio::setAVCodecContext(AVCodecContext * audioContext)
{
    this->audioContext = audioContext;
}

//获取播放状态
bool Audio::getIsPlaying()
{
    return isPlay;
}

//设置播放状态
void Audio::setPlaying(bool isPlaying)
{
    this->isPlay = isPlaying;
}

//清理音频包队列
void Audio::clearPacket()
{
    audiaPackets.queueFlush();
}

//设置音量
void Audio::setVolume(int volume)
{
    audioVolume = volume;
}

/**
* 向设备发送audio数据的回调函数
*/
void audioCallback(void* userdata, Uint8 *stream, int len)
{
    Audio  *audio = Media::getInstance()->audio;

    SDL_memset(stream, 0, len);

    int audio_size = 0;
    int len1 = 0;
    while(len > 0)// 向设备发送长度为len的数据
    {
        if(audio->getAudioBuffIndex() >= audio->getAudioBuffSize()) // 缓冲区中无数据
        {
            // 从packet中解码数据
            audio_size = audioDecodeFrame(audio, audio->getAudioBuff(), sizeof(audio->getAudioBuff()));
            if(audio_size < 0) // 没有解码到数据或出错，填充0
            {
                audio->setAudioBuffSize(0);
                memset(audio->getAudioBuff(), 0, audio->getAudioBuffSize());
            }
            else
                audio->setAudioBuffSize(audio_size);

            audio->setAudioBuffIndex(0);
        }
        len1 = audio->getAudioBuffSize() - audio->getAudioBuffIndex(); // 缓冲区中剩下的数据长度
        if(len1 > len) // 向设备发送的数据长度为len
            len1 = len;

        SDL_MixAudio(stream, audio->getAudioBuff() + audio->getAudioBuffIndex(), len, audioVolume);

        len -= len1;
        stream += len1;
        audio->setAudioBuffIndex(audio->getAudioBuffIndex()+len1);
    }
}

/**
* 解码Avpacket中的数据填充到缓冲空间
*/
int audioDecodeFrame(Audio*audio, uint8_t *audioBuffer, int bufferSize)
{
    AVFrame *frame = av_frame_alloc();
    int data_size = 0;
    AVPacket pkt = audio->dequeuePacket();
    SwrContext *swr_ctx = NULL;
    static double clock = 0;

    /*if (quit)
        return -1;*/
    if(pkt.size <= 0)
        return -1;

    if(pkt.pts != AV_NOPTS_VALUE)
    {
        if(audio->getStream() == NULL)
            return -1;
        audio->setAudioClock(av_q2d(audio->getStream()->time_base) * pkt.pts); //当前packet的播放时间
    }
    if(audio->getAVCodecContext() == NULL)
        return -1;
    int ret = avcodec_send_packet(audio->getAVCodecContext(), &pkt);
    if(ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
        return -1;

    ret = avcodec_receive_frame(audio->getAVCodecContext(), frame);
    if(ret < 0 && ret != AVERROR_EOF)
        return -1;
    int p = (frame->pts *r2d(audio->getStream()->time_base)) * 1000;
    Media::getInstance()->pts = p;
    // 设置通道数或通道布局
    if(frame->channels > 0 && frame->channel_layout == 0)
        frame->channel_layout = av_get_default_channel_layout(frame->channels); //根据通道的个数返回默认的layout
    else if(frame->channels == 0 && frame->channel_layout > 0)
        frame->channels = av_get_channel_layout_nb_channels(frame->channel_layout); //根据通道的layout返回通道的个数

    //音频sample的存储格式
    AVSampleFormat dst_format = AV_SAMPLE_FMT_S16;//av_get_packed_sample_fmt((AVSampleFormat)frame->format);
    Uint64 dst_layout = av_get_default_channel_layout(frame->channels);
    // 设置转换参数
    swr_ctx = swr_alloc_set_opts(NULL, dst_layout, dst_format, frame->sample_rate, //输出参数
        frame->channel_layout, (AVSampleFormat)frame->format, frame->sample_rate, 0, NULL);
    if(!swr_ctx || swr_init(swr_ctx) < 0) //初始化
        return -1;

    // 计算转换后的sample个数 a * b / c
    uint64_t dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples, frame->sample_rate, frame->sample_rate, AVRounding(1));
    // 转换，返回值为转换后的sample个数
    int nb = swr_convert(swr_ctx, &audioBuffer, static_cast<int>(dst_nb_samples), (const uint8_t**)frame->data, frame->nb_samples);
    data_size = frame->channels * nb * av_get_bytes_per_sample(dst_format);

    // 每秒钟音频播放的字节数 sample_rate * channels * sample_format(一个sample占用的字节数)
    if(audio->getStream()->codec == NULL)
        return -1;
    //当前播放时间,(audio_clock+当前缓冲区index)/每秒播放的字节(bytes)数x   x=每个样本大小 * 通道数 * 样本率
    audio->setAudioClock(audio->getAudioClock()+static_cast<double>(data_size) / (2 * audio->getStream()->codec->channels * audio->getStream()->codec->sample_rate));
    av_frame_free(&frame);
    swr_free(&swr_ctx);
    return data_size;
}
