#include "utils.h"
#include "platform.h"
#include "securetypes.h"

#include <ctime>

#include <QDesktopWidget>
#include <QFileInfo>

void memrandomset(byte* data, size_t size)
{
    srand((unsigned int)time(NULL));

    byte* dataEnd { data + size };

    while(data != dataEnd) {
        *data = rand()%256;
        ++data;
    }
}

void memrandomset(SecureBytes &bytes)
{
    memrandomset(as<byte*>(bytes), bytes.size());
}

int callQuestionDialog(const QString &message, QWidget *parent)
{
    QMessageBox msgBox {parent};
    msgBox.setText(message);

    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    return msgBox.exec();
}

void callInfoDialog(const QString &message, QWidget *parent)
{
    QMessageBox msgBox {parent};
    msgBox.setText(message);

    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

bool copyFileForced(const QString &from, const QString &to)
{
    if (QFileInfo{from} == QFileInfo{to}) {
        return true;
    }

    if (QFile::exists(to)) {
        QFile::remove(to);
    }
    return QFile::copy(from, to);
}

QString passGenerate(int n, PasswordType::type type)
{
    srand((unsigned int)time(NULL));

    QString symbols { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" };
    if(type > PasswordType::Letters) symbols += QStringLiteral("0123456789");
    if(type > PasswordType::LettersAndDigits) symbols += QStringLiteral(".,/-_");
    if(type > PasswordType::Standard) symbols += QStringLiteral("!#$%&()*+[]\\^{|}");

    QString pass;
    for(int i = 0; i<n; ++i) {
        pass += symbols[rand()%symbols.size()];
    }

    return pass;
}

void allignWindowToCenter(QWidget *w)
{
    QWidget *scr { QApplication::desktop()->screen(0) }; // 0 - screen No
    int scrWidth { scr->width() };
    int scrHeight {scr->height() };

    w->setGeometry((scrWidth/2) - (w->width()/2), (scrHeight/2) - (w->height()/2), w->width(), w->height());
}
