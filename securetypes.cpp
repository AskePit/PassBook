#include "securetypes.h"
#include "utils.h"
#include "crypt.h"
#include "hash.h"

#include <algorithm>
#include <functional>
#include <QPainter>
#include <QStyleOptionViewItem>

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

SecureBytes::SecureBytes(std::initializer_list<byte> list)
    : QByteArray(static_cast<int>(list.size()), 0)
{
    std::copy(list.begin(), list.end(), begin());
}

SecureBytes::~SecureBytes()
{
    wipememory(data(), size());
}

SecureBytes operator+(const SecureBytes &b1, const SecureBytes &b2)
{
    SecureBytes res(b1);
    res.append(b2);
    return res;
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
    int size { m_data.size() };
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
    memrandomset(m_x);
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

const SecureBytes &MasterDoor::get()
{
    return m_master.m_data;
}

static SecureBytes staticSalt {0x85, 0x54, 0xd6, 0xb0, 0x9f, 0x36, 0xaa};

SecureBytes MasterDoor::getHash(const SecureBytes &salt)
{
    SecureBytes salted = m_master.m_data + salt + staticSalt;
    SecureBytes hash(gost::SIZE_OF_HASH);

    gost::hash(as<byte*>(hash), as<byte*>(salted), salted.size());
    return hash;
}

HashAndSalt MasterDoor::getHash()
{
    SecureBytes salt(gost::SIZE_OF_SALT);
    memrandomset(salt);

    SecureBytes hash { getHash(salt) };

    return {hash, salt};
}

Password::Password(QString &&pass, const Master &master)
    : m_master(&master)
{
    load(std::move(pass), master);
}

void Password::load(QString &&pass, const Master &master)
{
    SecureBytes bytes(std::move(pass));

    int size { bytes.size() };
    m_cryptedPass.resize(size);

    gost::Crypter crypter;
    MasterDoor door(master);
    crypter.cryptData(as<byte*>(m_cryptedPass), as<byte*>(bytes), size, as<const byte*>(door.get()));

    m_master = &master;
    m_loaded = true;
}

void Password::reload(QString &&pass)
{
    load(std::move(pass), *m_master);
}

SecureString Password::get() const
{
    if(m_cryptedPass.isEmpty()) {
        return QString();
    }

    SecureBytes pass(m_cryptedPass.size());

    gost::Crypter crypter;
    MasterDoor door {*m_master};
    crypter.cryptData(as<byte*>(pass), as<const byte*>(m_cryptedPass), m_cryptedPass.size(), as<const byte*>(door.get()));

    return SecureString{ QString::fromUtf8(pass) };
}

void Password::paint(QPainter *painter, const QStyleOptionViewItem &option, bool show)
{
    const QRect &rect { option.rect };

    if(option.state & QStyle::State_Selected) {
        painter->fillRect(rect, QColor(0xF5, 0xF5, 0xF5));
    }

    if(m_cryptedPass.isEmpty()) {
        return;
    }

    if(show) {
        QString pass { get() };

        QFont font {"Consolas", 9};
        QFontMetrics fm {font};
        const int margin {4};
        int w { fm.width(pass) + margin };

        QPixmap pixmap {w, rect.height()};
        pixmap.fill();

        QPainter p(&pixmap);
        p.setFont(font);

        if(option.state & QStyle::State_Selected) {
            p.fillRect(0, 0, rect.width(), rect.height(), QColor(0xF5, 0xF5, 0xF5));
        }
        p.drawText(margin, margin, w, rect.height(), 0, pass);
        painter->drawPixmap(rect.x(), rect.y(), pixmap);
    } else {
        QPixmap pixmap {rect.width(), rect.height()};
        pixmap.fill();

        QPainter p {&pixmap};
        p.fillRect(0, 0, rect.width(), rect.height(), Qt::Dense4Pattern);
        painter->drawPixmap(option.rect.x(), rect.y(), pixmap);
    }
}
