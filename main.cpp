#include <QApplication>
#include "passworddialog.h"
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    /*QTextCodec::setCodecForCStrings(QTextCodec::codecForLocale());
    QTextCodec *codec = QTextCodec::codecForName("windows-1251");
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));*/

    passwordDialog w;
    w.show();

    return a.exec();

    return 0;
}
