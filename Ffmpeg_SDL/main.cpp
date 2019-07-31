#include "QTPlayer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTPlayer w;
    w.show();

    return a.exec();
}
