#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include "forms/dialogs/logindialog.h"

int main(int argc, char *argv[])
{
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication a(argc, argv);

    QFile themeFile(QStringLiteral(":/theme.qss"));
    if (themeFile.open(QFile::ReadOnly | QFile::Text)) {
        a.setStyleSheet(QString::fromUtf8(themeFile.readAll()));
    }

    LoginDialog *w { new LoginDialog };
    w->show();

    return a.exec();
}
