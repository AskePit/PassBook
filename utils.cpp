#include "utils.h"
#include "platform.h"

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

QString passGenerate(int n, int mode)
{
    srand((unsigned int)time(NULL));

    QString symbols = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    if(mode>1) symbols += "0123456789";
    if(mode>2) symbols += ".,/-_";
    if(mode>3) symbols += "!#$%&()*+[]\\^{|}";

    QString pass;
    for(int i = 0; i<n; i++) pass += symbols[rand()%symbols.size()];

    return pass;
}

void allignWindowToCenter(QWidget *w)
{
    QWidget *scr = QApplication::desktop()->screen(0); // 0 - screen No
    int scrWidth = scr->width();
    int scrHeight = scr->height();

    w->setGeometry((scrWidth/2) - (w->width()/2), (scrHeight/2) - (w->height()/2), w->width(), w->height());
}
