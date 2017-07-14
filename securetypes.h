#ifndef PASSWORDPIXMAP_H
#define PASSWORDPIXMAP_H

#include <QMetaType>
#include <QDataStream>
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

class QPainter;
class QStyleOptionViewItem;

class Password
{
public:
    Password() = default;
    Password(QString &&pass, const Master &master);

    void load(QString &&pass, const Master &master);
    void reload(QString &&pass);
    bool isLoaded() const { return m_loaded; }

    SecureString get() const;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, bool show);

    friend QDataStream &operator << (QDataStream &arch, const Password &object)
    {
        arch << object.m_loaded;
        arch << object.m_cryptedPass;
        arch << reinterpret_cast<ptrdiff_t>(object.m_master);
        return arch;
    }

    friend QDataStream &operator >> (QDataStream &arch, Password &object)
    {
        ptrdiff_t masterAddr;

        arch >> object.m_loaded;
        arch >> object.m_cryptedPass;
        arch >> masterAddr;

        object.m_master = reinterpret_cast<const Master*>(masterAddr);
        return arch;
    }

private:
    bool m_loaded = false;
    SecureBytes m_cryptedPass;
    const Master *m_master;
};

Q_DECLARE_METATYPE(Password)

#endif // PASSWORDPIXMAP_H
