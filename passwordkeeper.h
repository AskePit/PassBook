#ifndef PASSWORDPIXMAP_H
#define PASSWORDPIXMAP_H

#include <QString>
#include "platform.h"


class PasswordKeeper
{
public:
    PasswordKeeper();
    PasswordKeeper(const PasswordKeeper& other);
    PasswordKeeper(QString pass, const byte* masterPass);
    ~PasswordKeeper();

    PasswordKeeper& operator=(const PasswordKeeper& other);

    void load(QString pass, const byte* masterPass);
    bool isLoaded() const { return loaded; }

    QString getPass(const byte* masterPass) const;

private:
    bool loaded;
    byte* cryptedPass;
    size_t sizeOfPass;
};

#endif // PASSWORDPIXMAP_H
