#include <QApplication>
#include "dialogs/passworddialog.h"
#include "securetypes.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaTypeStreamOperators<Password>("Password");

    PasswordDialog *w { new PasswordDialog };
    w->show();

    return a.exec();
}
