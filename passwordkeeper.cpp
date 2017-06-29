#include "passwordkeeper.h"
#include "Crypt.h"

PasswordKeeper::PasswordKeeper()
    : loaded(false)
    , cryptedPass(0)
    , sizeOfPass(0)
{
}

PasswordKeeper::PasswordKeeper(const PasswordKeeper& other)
{
    (*this) = other;
}

PasswordKeeper::PasswordKeeper(QString pass, const byte* masterPass)
    : loaded(true)
    , cryptedPass(0)
    , sizeOfPass(0)
{
    load(pass, masterPass);
}

PasswordKeeper::~PasswordKeeper()
{
    delete [] cryptedPass;
}

PasswordKeeper& PasswordKeeper::operator=(const PasswordKeeper& other)
{
    loaded = other.loaded;
    sizeOfPass = other.sizeOfPass;
    cryptedPass = new byte[sizeOfPass];
    memcpy(cryptedPass, other.cryptedPass, sizeOfPass);

    return (*this);
}

void PasswordKeeper::load(QString pass, const byte* masterPass)
{
    loaded = true;
    if(cryptedPass != 0) delete [] cryptedPass;
    sizeOfPass = pass.length();
    cryptedPass = new byte[sizeOfPass];
    gost::Crypter crypter;
    crypter.cryptString(cryptedPass, pass.toStdString().c_str(), masterPass);
}

QString PasswordKeeper::getPass(const byte* masterPass) const
{
    char* pass = new char[sizeOfPass + 1];

    gost::Crypter crypter;
    crypter.decryptString(pass, cryptedPass, sizeOfPass, masterPass);

    QString result(pass);
    delete [] pass;
    return result;
}
