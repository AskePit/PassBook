#include "utils.h"
#include "platform.h"
#include "securetypes.h"

#include <ctime>
#include <cstdlib>

#include <QDesktopWidget>
#include <QApplication>

void memrandomset(byte* data, size_t size)
{
    srand((unsigned int)time(NULL));

    byte* dataEnd = data + size;

    while(data != dataEnd) {
        *data = rand()%256;
        ++data;
    }
}

void memrandomset(SecureBytes &bytes)
{
    memrandomset(as<byte*>(bytes), bytes.size());
}

QString passGenerate(int n, PasswordType::type type)
{
    srand((unsigned int)time(NULL));

    QString symbols = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    if(type > PasswordType::Letters) symbols += "0123456789";
    if(type > PasswordType::LettersAndDigits) symbols += ".,/-_";
    if(type > PasswordType::Standard) symbols += "!#$%&()*+[]\\^{|}";

    QString pass;
    for(int i = 0; i<n; ++i) {
        pass += symbols[rand()%symbols.size()];
    }

    return pass;
}

void allignWindowToCenter(QWidget *w)
{
    QWidget *scr = QApplication::desktop()->screen(0); // 0 - screen No
    int scrWidth = scr->width();
    int scrHeight = scr->height();

    w->setGeometry((scrWidth/2) - (w->width()/2), (scrHeight/2) - (w->height()/2), w->width(), w->height());
}
