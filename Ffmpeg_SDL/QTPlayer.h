#ifndef QTPLAYER_H
#define QTPLAYER_H

#include <QWidget>
#include <QMenu>
#include <QAction>
#include <QPropertyAnimation>
#include <QImage>


namespace Ui{
class QTPlayer;
}

class QTPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit QTPlayer(QWidget *parent = 0);
    ~QTPlayer();

    void resizeEvent(QResizeEvent *e);
    void contextMenuEvent(QContextMenuEvent *event);
    void mirrorLeftAndRight();
    void mirrorUpAndDown();
    void gray2Rgb();
    void rgb2Gray();
    void netAddressInput();
    void openNetAddressVideo(QString address);
    void mouseDoubleClickEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent *event);

public slots:
    void openVideoFile();
    void timerEvent(QTimerEvent *e);
    void sliderPress();
    void sliderRelease();
    void play();
    void setVideoImage(QImage*);
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_volume_button_clicked();
    void on_verticalSlider_valueChanged(int value);

    bool eventFilter(QObject *obj, QEvent *event);

signals:
    void sendPos(float pos);

private:
    QPropertyAnimation *bottomAnimation;
    bool isFullScreen = false;
    void showBottomInAnimation();
    void hideBottomInAnimation();
    QMenu *popMenu;
    QAction *leftRightMirrorAction;
    QAction *upDownMirrorAction;
    QAction *rgbAction;
    QAction *grayAction;
    QAction *netAddressAction;

    int total;  //视频总时长
    int _pos;

    Ui::QTPlayer *ui;
};

#endif // QTPLAYER_H
