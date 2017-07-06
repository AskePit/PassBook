#ifndef PASSWORDPIXMAP_H
#define PASSWORDPIXMAP_H

#include <QString>
#include <vector>
#include "platform.h"
#include "utils.h"

template <typename T>
class Secure
{
public:
    Secure(const T &data)
        : m_data(data)
    {}

    ~Secure() {
        wipememory(m_data.data(), m_data.size());
    }

private:
    const T &m_data;
};

template <>
class Secure<QString>
{
public:
    Secure(QString &data)
        : m_data(data)
    {}

    ~Secure() {
        wipememory(m_data.constData(), m_data.size()*2); // QChar is 16 bytes
    }

private:
    QString &m_data;
};

class Master
{
public:
    explicit Master(QString &&key);
    ~Master();

private:
    QByteArray m_data;
    QByteArray m_x;

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
    Password(const QString &pass, Master &master);

    void load(const QString &pass, Master &master);
    bool isLoaded() const { return m_loaded; }

    QString get(Master &master) const;

private:
    bool m_loaded = false;
    QByteArray m_cryptedPass;
};

#endif // PASSWORDPIXMAP_H
