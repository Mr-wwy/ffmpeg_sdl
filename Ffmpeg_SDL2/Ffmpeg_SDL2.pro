#-------------------------------------------------
#
# Project created by QtCreator 2018-12-29T15:04:03
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Ffmpeg_SDL2
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH +=./include/ffmpeg  ./include/sdl

LIBS += -L$$PWD/lib/ffmpeg -lavformat \
-lavcodec \
-lavutil \
-lswscale \
-lswresample \
-lavdevice \
-lavfilter \
-L$$PWD/lib/SDL -lSDL2
#/usr/local/lib/libSDL2.so


SOURCES += \
        main.cpp \
        QTPlayer.cpp \
    Audio.cpp \
    Video.cpp \
    PacketQueue.cpp \
    FrameQueue.cpp \
    DisplayMediaTimer.cpp \
    ImageFilter.cpp \
    QImageHandler.cpp \
    Media.cpp

HEADERS += \
        QTPlayer.h \
    Audio.h \
    Video.h \
    PacketQueue.h \
    FrameQueue.h \
    DisplayMediaTimer.h \
    ImageFilter.h \
    QImageHandler.h \
    Media.h

FORMS += \
        QTPlayer.ui

RESOURCES += \
    QTPlayer.qrc
