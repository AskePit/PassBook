#ifndef PASSWORDPIXMAP_H
#define PASSWORDPIXMAP_H

#include <QString>
#include "platform.h"

class SecureString : public QString
{
public:
    SecureString(QString &&str);
    ~SecureString();
};

class SecureBytes : public QByteArray
{
public:
    SecureBytes();
    SecureBytes(int size, char c);
    explicit SecureBytes(SecureString &&str);
    SecureBytes(QByteArray &&bytes);

    ~SecureBytes();
};

class Master
{
public:
    explicit Master(SecureString &&key);
    explicit Master(SecureBytes &&key);
    ~Master();

private:
    SecureBytes m_data;
    SecureBytes m_x;

    void init();
    void lock();
    void unlock();
    void transform();
    friend class MasterDoor;
};

class MasterDoor {
public:
    explicit MasterDoor(Master &master);
    ~MasterDoor();
    byte *get();
private:
    Master &m_master;
};

class Password
{
public:
    Password() = default;
    Password(QString &&pass, Master &master);

    void load(QString &&pass, Master &master);
    bool isLoaded() const { return m_loaded; }

    SecureString get(Master &master) const;

private:
    bool m_loaded = false;
    SecureBytes m_cryptedPass;
};

#endif // PASSWORDPIXMAP_H
