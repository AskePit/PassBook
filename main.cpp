#include <QApplication>
#include "dialogs/logindialog.h"
#include "securetypes.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaTypeStreamOperators<Password>("Password");

    LoginDialog *w { new LoginDialog };
    w->show();

    return a.exec();
}
