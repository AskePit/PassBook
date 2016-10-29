#ifndef INSTRUMENTS_H
#define INSTRUMENTS_H

#include <QString>

using namespace std;

class QWidget;

void memrandomset(void* data, size_t size);
QString passGenerate(int n, int mode);
void allignWindowToCenter(QWidget *w);

#endif //INSTRUMENTS_H
