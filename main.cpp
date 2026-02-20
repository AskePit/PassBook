#include <QApplication>
#include <QStyleFactory>
#include "forms/dialogs/logindialog.h"

int main(int argc, char *argv[])
{
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication a(argc, argv);

    LoginDialog *w { new LoginDialog };
    w->show();

    return a.exec();
}
