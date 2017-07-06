#include "securetypes.h"
#include "utils.h"
#include "crypt.h"

#include <algorithm>

SecureString::SecureString(QString &&str) : QString(str) {}

SecureString::~SecureString()
{
    wipememory(constData(), size()*2); // QChar is 16 bytes
}

SecureBytes::SecureBytes() : QByteArray() {}
SecureBytes::SecureBytes(int size, char c) : QByteArray(size, c) {}
SecureBytes::SecureBytes(SecureString &&str) : QByteArray(str.toUtf8()) {}
SecureBytes::SecureBytes(QByteArray &&bytes) : QByteArray(bytes) {}

SecureBytes::~SecureBytes()
{
    wipememory(data(), size());
}

Master::Master(SecureString &&key)
    : m_data(key.toUtf8())
    , m_x(gost::SIZE_OF_KEY, 0)
{
    init();
}

Master::Master(SecureBytes &&key)
    : m_data(key)
    , m_x(gost::SIZE_OF_KEY, 0)
{
    init();
}

void Master::init()
{
    int size = m_data.size();
    m_data.resize(gost::SIZE_OF_KEY);

    if(size < gost::SIZE_OF_KEY) {
        wipememory(m_data.data() + size, gost::SIZE_OF_KEY - size);
    }

    lock();
}

Master::~Master()
{
    wipememory(as_bytes(m_data), gost::SIZE_OF_KEY);
}

void Master::lock()
{
    memrandomset(as_bytes(m_x), gost::SIZE_OF_KEY);
    transform();
}

void Master::unlock()
{
    transform();
}

void Master::transform()
{
    for(int i = 0; i<gost::SIZE_OF_KEY; ++i) {
        m_data[i] = m_data[i] ^ m_x[i];
    }
}

MasterDoor::MasterDoor(Master &master)
    : m_master(master)
{
    m_master.unlock();
}

MasterDoor::~MasterDoor()
{
    m_master.lock();
}

byte *MasterDoor::get()
{
    return as_bytes(m_master.m_data.data());
}

Password::Password(QString &&pass, Master &master)
{
    load(std::move(pass), master);
}

void Password::load(QString &&pass, Master &master)
{
    SecureBytes bytes(std::move(pass));

    int size = bytes.size();
    m_cryptedPass.resize(size);

    {
        gost::Crypter crypter;
        MasterDoor door(master);
        crypter.cryptData(as_bytes(m_cryptedPass), as_bytes(bytes), size, door.get());
    }

    m_loaded = true;
}

SecureString Password::get(Master &master) const
{
    if(m_cryptedPass.isEmpty()) {
        return QString();
    }

    QByteArray pass(m_cryptedPass.size(), 0);

    {
        gost::Crypter crypter;
        MasterDoor door(master);
        crypter.cryptData(as_bytes(pass), as_bytes(m_cryptedPass), m_cryptedPass.size(), door.get());
    }

    return QString::fromUtf8(pass);
}
