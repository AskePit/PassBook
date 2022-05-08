#include <QApplication>
#include "dialogs/logindialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    LoginDialog *w { new LoginDialog };
    w->show();

    return a.exec();
}
