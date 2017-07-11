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
    SecureBytes(int size);
    explicit SecureBytes(SecureString &&str);
    SecureBytes(std::initializer_list<byte>);
    SecureBytes(QByteArray &&bytes);

    ~SecureBytes();
};

SecureBytes operator+(const SecureBytes &b1, const SecureBytes &b2);

class Master
{
public:
    explicit Master(SecureString &&key);
    explicit Master(SecureBytes &&key);
    ~Master();

private:
    mutable SecureBytes m_data;
    mutable SecureBytes m_x;

    void init();
    void lock() const;
    void unlock() const;
    friend class MasterDoor;
};

struct HashAndSalt {
    SecureBytes hash;
    SecureBytes salt;
};

class MasterDoor {
public:
    explicit MasterDoor(const Master &master);
    ~MasterDoor();
    const SecureBytes &get();
    SecureBytes getHash(const SecureBytes &salt);
    HashAndSalt getHash();
private:
    const Master &m_master;
};

class Password
{
public:
    Password() = default;
    Password(QString &&pass, const Master &master);

    void load(QString &&pass, const Master &master);
    bool isLoaded() const { return m_loaded; }

    SecureString get(const Master &master) const;

private:
    bool m_loaded = false;
    SecureBytes m_cryptedPass;
};

#endif // PASSWORDPIXMAP_H
