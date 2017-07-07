#include "securetypes.h"
#include "utils.h"
#include "crypt.h"

#include <algorithm>
#include <functional>

SecureString::SecureString(QString &&str) : QString(str) {}

SecureString::~SecureString()
{
    // QString::data() will trigger COW, so use QString::constData()
    wipememory(constData(), size()*2); // QChar is 16 bytes
}

SecureBytes::SecureBytes() : QByteArray() {}
SecureBytes::SecureBytes(int size) : QByteArray(size, 0) {}
SecureBytes::SecureBytes(SecureString &&str) : QByteArray(str.toUtf8()) {}
SecureBytes::SecureBytes(QByteArray &&bytes) : QByteArray(bytes) {}

SecureBytes::~SecureBytes()
{
    wipememory(data(), size());
}

Master::Master(SecureString &&key)
    : m_data(key.toUtf8())
    , m_x(gost::SIZE_OF_KEY)
{
    init();
}

Master::Master(SecureBytes &&key)
    : m_data(key)
    , m_x(gost::SIZE_OF_KEY)
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
    wipememory(as<byte*>(m_data), gost::SIZE_OF_KEY);
}

static inline void xor_arrays(QByteArray &to, const QByteArray &from)
{
    using namespace std;

    std::transform(
        begin(to), end(to),
        begin(from),
        begin(to),
        bit_xor<byte>()
    );
}

void Master::lock() const
{
    memrandomset(as<byte*>(m_x), gost::SIZE_OF_KEY);
    xor_arrays(m_data, m_x);
}

void Master::unlock() const
{
    xor_arrays(m_data, m_x);
}

MasterDoor::MasterDoor(const Master &master)
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
    return as<byte*>(m_master.m_data);
}

Password::Password(QString &&pass, const Master &master)
{
    load(std::move(pass), master);
}

void Password::load(QString &&pass, const Master &master)
{
    SecureBytes bytes(std::move(pass));

    int size = bytes.size();
    m_cryptedPass.resize(size);

    gost::Crypter crypter;
    MasterDoor door(master);
    crypter.cryptData(as<byte*>(m_cryptedPass), as<byte*>(bytes), size, door.get());

    m_loaded = true;
}

SecureString Password::get(const Master &master) const
{
    if(m_cryptedPass.isEmpty()) {
        return QString();
    }

    SecureBytes pass(m_cryptedPass.size());

    gost::Crypter crypter;
    MasterDoor door(master);
    crypter.cryptData(as<byte*>(pass), as<const byte*>(m_cryptedPass), m_cryptedPass.size(), door.get());

    return SecureString( QString::fromUtf8(pass) );
}
