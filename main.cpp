#include <QApplication>
#include "dialogs/passworddialog.h"
#include <QTextCodec>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    PasswordDialog w;
    w.show();

    return a.exec();
}
