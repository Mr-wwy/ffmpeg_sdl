#include "QTPlayer.h"
#include "ui_QTPlayer.h"
#include "DisplayMediaTimer.h"
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include "ImageFilter.h"
#include "Media.h"
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QPixmap>
#include <QDebug>

static bool isPressSlider = false;  //进度条鼠标拖动松开判断
static bool isPlay = false;     //视频播放判断
static bool flag = false;       //为播放视频时播放按钮点击无效判断
static bool volume = true;      //开关音量控制判断

QTPlayer::QTPlayer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QTPlayer)
{
    ui->setupUi(this);

    connect(DisplayMediaTimer::getInstance(),SIGNAL(updateFrame(QImage*)),
            this,SLOT(setVideoImage(QImage*)));
    connect(this,SIGNAL(sendPos(float)),
        Media::getInstance(),SLOT(receivePos(float)));

    setMinimumWidth(400);
    setMinimumHeight(300);
    setWindowIcon(QIcon(":/new/12"));
    bottomAnimation = new QPropertyAnimation(ui->bottemWidget, "geometry"); //动画
    setMouseTracking(true); //设置窗口部件跟踪鼠标生效
    ui->label->setMouseTracking(true);
    popMenu = new QMenu(this);
    leftRightMirrorAction = new QAction(this);
    upDownMirrorAction = new QAction(this);
    rgbAction = new QAction(this);
    grayAction = new QAction(this);
    netAddressAction = new QAction(this);
    leftRightMirrorAction->setText(tr("左右镜像"));
    upDownMirrorAction->setText(tr("上下镜像"));
    rgbAction->setText(QString::fromLocal8Bit("彩色"));
    grayAction->setText(tr("黑白"));
    netAddressAction->setText(tr("网络串流"));
    connect(leftRightMirrorAction, &QAction::triggered, this, &QTPlayer::mirrorLeftAndRight);
    connect(upDownMirrorAction, &QAction::triggered, this, &QTPlayer::mirrorUpAndDown);
    connect(rgbAction, &QAction::triggered, this, &QTPlayer::gray2Rgb);
    connect(grayAction, &QAction::triggered, this, &QTPlayer::rgb2Gray);
    connect(netAddressAction, &QAction::triggered, this, &QTPlayer::netAddressInput);

    ui->verticalSlider->setVisible(false);
    ui->current_time->setVisible(false);
    ui->label_2->setVisible(false);
    ui->label_3->setVisible(false);
    ui->playslider->installEventFilter(this);   //安装事件过滤器
    ui->verticalSlider->installEventFilter(this);

    startTimer(40);

    ui->playButton->setStyleSheet("QPushButton{border-image:url(:/new/10);}");
}

//获取网路转播地址
void QTPlayer::netAddressInput()
{
    bool ok = false;

    QString text = QInputDialog::getText( this,
        tr("网络流"),tr("请输入地址"),
        QLineEdit::Normal, QString::null, &ok);
    if(ok&&!text.isEmpty())
        openNetAddressVideo(text);
    else
        ;// 用户按下Cancel

}

//程序入口，把视频地址传给media开始工作
void QTPlayer::openNetAddressVideo(QString address)
{
    std::string file = address.toLocal8Bit().data();//防止有中文
    Media *media = Media::getInstance()
        ->setMediaFile(file.c_str())
        ->config();

    if(!media)
    {
        QMessageBox::warning(this,tr("warning"),tr("转播地址错误或网络未连接"));
        return;
    }
    media->start();
    Media::getInstance()->video->start();
    setWindowTitle(address);
    total = Media::getInstance()->totalMs;
    ui->totalHour->setText(QString::number((int)(total / 1000 / 60 / 60)) + ":");//计算视频总的时分秒
    ui->totalMinute->setText(QString::number((int)(total / 1000 / 60) % 60) + ":");
    ui->totalSecond->setText(QString::number((int)(total / 1000) % 60 % 60));
    flag = true;
    isPlay = false;
    play();
    float pos = 0;
    pos = (float)ui->verticalSlider->value() / (float)(ui->verticalSlider->maximum() + 1);
    if(Media::getInstance()->getAVFormatContext() && Media::getInstance()->audio->getStreamIndex() >= 0)
        Media::getInstance()->audio->setVolume(pos*SDL_MIX_MAXVOLUME);//初始化音量
}

//右击弹出菜单
void QTPlayer::contextMenuEvent(QContextMenuEvent *event)
{
    popMenu->clear();
    popMenu->addAction(leftRightMirrorAction);
    popMenu->addAction(upDownMirrorAction);
    popMenu->addAction(rgbAction);
    popMenu->addAction(grayAction);
    popMenu->addAction(netAddressAction);

    //菜单出现的位置为当前鼠标的位置
    popMenu->exec(QCursor::pos());
    event->accept();
}

//鼠标双击最大化最小化
void QTPlayer::mouseDoubleClickEvent(QMouseEvent * event)
{
    if(windowState() &  Qt::WindowFullScreen)
        showNormal();
    else
        showFullScreen();
}

//隐藏，显示底部
void QTPlayer::mouseMoveEvent(QMouseEvent *event)
{
    int y = event->globalY();
    if(y >= this->height()*0.9)
        showBottomInAnimation();
    else
    {
        hideBottomInAnimation();
        if(!volume) //如果音量控制滑动条没关闭就关闭它
        {
            ui->verticalSlider->setVisible(false);
            volume = !volume;
        }
    }
}

//选择打开视频文件
void QTPlayer::openVideoFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
       "please select your video file!",".","*.mp4 *.rmvb *.flv *.avi *.mkv");
    if(fileName.isEmpty())
        return;
/*    QStringList titles = fileName.split("/");
    QString postfix = titles.constLast().split(".").last();
    if(postfix != QString::fromLocal8Bit("mp4")
        && postfix != QString::fromLocal8Bit("rmvb")
        && postfix != QString::fromLocal8Bit("flv")
        && postfix != QString::fromLocal8Bit("avi")
        && postfix != QString::fromLocal8Bit("mkv")
        ){
        return;
    }*/
    openNetAddressVideo(fileName);
}

//设置音量大小,音量大小0-128
void QTPlayer::on_verticalSlider_valueChanged(int volume)
{
    if(!ui->verticalSlider->value())
        ui->volume_button->setStyleSheet("QPushButton{border-image: url(:/new/volume2)}");
    else
        ui->volume_button->setStyleSheet("QPushButton{border-image: url(:/new/volume)}");
    float pos = 0;
    pos = (float)ui->verticalSlider->value() / (float)(ui->verticalSlider->maximum() + 1);
    if(Media::getInstance()->getAVFormatContext() && Media::getInstance()->audio->getStreamIndex() >= 0)
        Media::getInstance()->audio->setVolume(pos*SDL_MIX_MAXVOLUME);
}

//底部显示动画效果
void QTPlayer::showBottomInAnimation()
{
    if(ui->bottemWidget->y() == height())
    {
        bottomAnimation->setDuration(500);
        bottomAnimation->setStartValue(QRect(ui->bottemWidget->x(), height(), ui->bottemWidget->width(), ui->bottemWidget->height()));

        bottomAnimation->setEndValue(QRect(ui->bottemWidget->x(), this->height() - ui->bottemWidget->height(), ui->bottemWidget->width(), ui->bottemWidget->height()));
        bottomAnimation->start();
    }
}

//底部隐藏动画效果
void QTPlayer::hideBottomInAnimation()
{
    if(ui->bottemWidget->y() == this->height() - ui->bottemWidget->height())
    {
        bottomAnimation->setDuration(500);
        bottomAnimation->setStartValue(QRect(ui->bottemWidget->x(), this->height() - ui->bottemWidget->height(), ui->bottemWidget->width(), ui->bottemWidget->height()));

        bottomAnimation->setEndValue(QRect(ui->bottemWidget->x(), this->height(), ui->bottemWidget->width(), ui->bottemWidget->height()));
        bottomAnimation->start();
    }
}

//设置进度条和播放时间
void QTPlayer::timerEvent(QTimerEvent * e)
{
    if(Media::getInstance()->totalMs > 0)
    {
        double pts = (double)Media::getInstance()->pts * 1000;
        double total = (double)Media::getInstance()->totalMs;
        double rate = pts / total;
        if (!isPressSlider&&isPlay) {
            ui->playslider->setValue(rate);
            ui->currentHour->setText(QString::number((int)(pts / 1000000 / 60 / 60))+":");
            ui->currentMinute->setText(QString::number((int)(pts / 1000000 / 60) % 60) + ":");
            ui->currentSecond->setText(QString::number((int)(pts / 1000000) % 60 % 60));
        }
    }
    if(!Media::getInstance()->getIsPlaying())
    {
        ui->label->clear();
        ui->label->setStyleSheet("QLabel{background-color: rgb(170, 170, 127);}");
    }
}

//拦截playslider verticalSlider上的鼠标点击事件,使滑动条点到即到
bool QTPlayer::eventFilter(QObject *obj, QEvent *event)
{
    if(obj==ui->playslider && Media::getInstance()->getIsPlaying())
    {
        if(event->type()==QEvent::MouseButtonPress)           //判断类型,鼠标按下
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) //判断左键
            {
               int dur = ui->playslider->maximum() - ui->playslider->minimum();
               int pos = ui->playslider->minimum() + dur * ((double)mouseEvent->x() / ui->playslider->width());
               if(pos != ui->playslider->sliderPosition())
                  ui->playslider->setValue(pos);    //设置进度条点到即到
            }
        }
        else if(event->type()==QEvent::MouseMove)   //鼠标点击拖动,鼠标指针上面显示拖到位置的视频时间
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            int pos = (double)mouseEvent->x()/ui->playslider->width()*total;
            ui->current_time->setText(QString::number((int)(pos / 1000 / 60 / 60)) + ":");
            ui->label_2->setText(QString::number((int)(pos / 1000 / 60) % 60) + ":");
            ui->label_3->setText(QString::number((int)(pos / 1000) % 60 % 60));
            ui->current_time->move(mouseEvent->x(),ui->playslider->y()-10);
            ui->label_2->move(mouseEvent->x()+15,ui->playslider->y()-10);
            ui->label_3->move(mouseEvent->x()+30,ui->playslider->y()-10);
            ui->current_time->setVisible(true);
            ui->label_2->setVisible(true);
            ui->label_3->setVisible(true);
        }
        else if(event->type()==QEvent::MouseButtonRelease)  //松开鼠标隐藏时间
        {
            ui->current_time->setVisible(false);
            ui->label_2->setVisible(false);
            ui->label_3->setVisible(false);
        }
    }
    else if(obj==ui->verticalSlider)
    {
        if(event->type()==QEvent::MouseButtonPress)           //判断类型
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if(mouseEvent->button() == Qt::LeftButton) //判断左键
            {
               int dur = ui->verticalSlider->maximum() - ui->verticalSlider->minimum();
               int pos = ui->verticalSlider->maximum() - dur * ((double)mouseEvent->y() / ui->verticalSlider->height());
               if(pos != ui->verticalSlider->sliderPosition())
                  ui->verticalSlider->setValue(pos);    //设置音量滑动条点到即到
            }
        }
    }

    return QObject::eventFilter(obj,event);
}

//按下进度条
void QTPlayer::sliderPress()
{
    if(!isPlay)
        return;
    isPressSlider = true;
}

//进度条释放
void QTPlayer::sliderRelease()
{
    if(!Media::getInstance()->getIsPlaying())
        return;
    isPressSlider = false;
    float pos = 0;
    pos = (float)ui->playslider->value() / (float)(ui->playslider->maximum() + 1);
    if(Media::getInstance()->audio->getStreamIndex() >= 0)
        emit sendPos(pos);
}

//视频播放,暂停
void QTPlayer::play()
{
    if(!flag)
        return;
    isPlay = !isPlay;
    DisplayMediaTimer::getInstance()->setPlay(isPlay);
    if(isPlay)
    {
        ui->playButton->setStyleSheet("QPushButton{border-image:url(:/new/11);}");
        if(Media::getInstance()->getAVFormatContext() && Media::getInstance()->audio->getStreamIndex() >= 0)
            SDL_PauseAudio(0); //继续Callback(),播放声音
            //Media::getInstance()->audio->audioPlay();
    }
    else
    {
        ui->playButton->setStyleSheet("QPushButton{border-image:url(:/new/10);}");
        if(Media::getInstance()->getAVFormatContext() && Media::getInstance()->audio->getStreamIndex() >= 0)
            SDL_PauseAudio(1); //退出Callback(),暂停声音
            //SDL_CloseAudio();
    }
}

//上下镜像操作
void QTPlayer::mirrorUpAndDown()
{
    if(Media::getInstance()->getAVFormatContext() && isPlay)
        ImageFilter::getInstance()->addTask(XTask{ XTASK_MIRRORUPANDDOWN });
}

//左右镜像操作
void QTPlayer::mirrorLeftAndRight()
{
    if(Media::getInstance()->getAVFormatContext() && isPlay)
        ImageFilter::getInstance()->addTask(XTask{ XTASK_MIRRORLEFTANDRIGHT });
}

//灰度图转rgb
void QTPlayer::gray2Rgb()
{
    if(Media::getInstance()->getAVFormatContext() && isPlay)
        ImageFilter::getInstance()->addColorTask(ColorTask{ COLRTASK_GRAY_TO_RGBA });
}

//rgb图转灰度图
void QTPlayer::rgb2Gray()
{
    if(Media::getInstance()->getAVFormatContext() && isPlay) {
        //for(int i=0;i<5;i++)
        ImageFilter::getInstance()->addColorTask(ColorTask{ COLRTASK_RGBA_TO_GRAY });
    }
}

//显示图像
void QTPlayer::setVideoImage(QImage*img)
{
    if(Media::getInstance()->getIsPlaying())
        ui->label->setPixmap(QPixmap::fromImage(*img));
}

//窗体改变大小时，适配所有控件，以播放按钮为基准
void QTPlayer::resizeEvent(QResizeEvent *e)
{
//    ui->label->resize(size());
    ui->label->setGeometry(0,this->height()*0.1,this->width(),this->height()*0.9);
    ui->bottemWidget->resize(this->width(), this->height()*0.1);
    ui->bottemWidget->move(0,this->height() - ui->bottemWidget->height());

    ui->playButton->move((ui->bottemWidget->width()- ui->playButton->width())/2, (ui->bottemWidget->height() - ui->playButton->height())/2+10);
    ui->openButton->move(ui->playButton->x()+ ui->playButton->width()+30, ui->playButton->y());

    ui->playslider->move(25, ui->playButton->y()-20);
    ui->playslider->resize(ui->bottemWidget->width() - 50, ui->playslider->height());

    ui->totalLayout->setSpacing(0);
    ui->totalContainer->setLayout(ui->totalLayout);
    ui->totalContainer->move(ui->playslider->x() + ui->playslider->width() - ui->totalContainer->width()-10, ui->playslider->y()+18);

    ui->currentLayout->setSpacing(0);
    ui->currentContainer->setLayout(ui->currentLayout);
    ui->currentContainer->resize(ui->totalContainer->size());
    ui->currentContainer->move(ui->playslider->x()+10, ui->playslider->y() + 18);

    ui->volume_button->move(ui->openButton->x()+200, ui->openButton->y()+5);
    ui->verticalSlider->move(ui->openButton->x()+205,this->height()-135);
    DisplayMediaTimer::getInstance()->resetImage(ui->label->width(), ui->label->height());
}

QTPlayer::~QTPlayer()
{
    delete ui;
}

void QTPlayer::on_pushButton_clicked()
{
    QString str = "rtmp://58.200.131.2:1935/livetv/hunantv";
    openNetAddressVideo(str);
}

void QTPlayer::on_pushButton_2_clicked()
{
    QString str = "rtmp://202.69.69.180:443/webcast/bshdlive-pc";
    openNetAddressVideo(str);
}

void QTPlayer::on_pushButton_3_clicked()
{
    QString str = "/home/abc/vedio/2.mp4";
    openNetAddressVideo(str);
}

void QTPlayer::on_volume_button_clicked()
{
    if(volume)
    {
        ui->verticalSlider->setVisible(true);
    }
    else
    {
        ui->verticalSlider->setVisible(false);
    }
    volume = !volume;
}
