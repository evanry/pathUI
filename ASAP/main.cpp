#include "pathologyworkstation.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{    
    QApplication a(argc, argv);
    QTranslator tran;
    bool ok = tran.load("qt_zh_CN.qm","D:\\");
    a.installTranslator(&tran);
    PathologyWorkstation w;
    w.show();

    return a.exec();
}
