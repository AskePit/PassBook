#include "passbook.h"
#include "crypt.h"
#include "hash.h"
#include "utils.h"
#include <QFileInfo>
#include <QFontMetrics>
#include <QPixmap>
#include <QPainter>

PassBook::PassBook(const QString &fileName, const Master &master)
    : m_loaded(false)
    , m_master(master)
    , m_fileName(fileName)
    , m_changed(false)
{}

static inline quint64 fileSize(const QString &fileName)
{
    QFileInfo info(fileName);
    return info.size();
}

int PassBook::verify()
{
    using namespace gost;

    quint64 sizeofFile = fileSize(m_fileName);

    if(sizeofFile < SIZE_OF_HASH + SIZE_OF_SALT) {
        return false;
    }

    SecureBytes fileHash(SIZE_OF_HASH);
    SecureBytes fileSalt(SIZE_OF_SALT);

    QFile in(m_fileName);
    in.open(QIODevice::ReadOnly);
    in.read(as<char *>(fileHash), SIZE_OF_HASH);
    in.read(as<char *>(fileSalt), SIZE_OF_SALT);
    in.close();

    MasterDoor door(m_master);
    SecureBytes realHash = door.getHash(fileSalt);

    return fileHash == realHash ? static_cast<int>(sizeofFile - SIZE_OF_HASH - SIZE_OF_SALT)
                                : -1;
}

static const char SOURCE_END = 0x7;
static const char URL_END    = 0x8;
static const char LOGIN_END  = 0x9;
static const char PASS_END   = 0xA;

static void parseData(const SecureBytes &data, QList<Note> &notes, const Master &master)
{
    const char* head = data.data();
    const char* cursor = head;
    const char* end = head + data.size();
    Note note;

    while(cursor != end) {
        if(*cursor < SOURCE_END || *cursor > PASS_END) {
            ++cursor;
            continue;
        }

        byte code = *cursor;

        SecureBytes fieldData(data.mid(head - data.data(), cursor - head));
        QString str(std::move(fieldData));

        switch(code) {
            case SOURCE_END: note.source = str; break;
            case URL_END:    note.URL    = str; break;
            case LOGIN_END:  note.login  = str; break;
            case PASS_END:   note.password.load(std::move(str), master);
                             notes.push_back(note);
                             break;
            default: break;
        }

        head = ++cursor;
    }
}

bool PassBook::load()
{
    using namespace gost;

    int sizeofMessage = verify();
    if(sizeofMessage < 0) {
        return m_loaded = false;
    }

    QFile f(m_fileName);
    f.open(QIODevice::ReadOnly);
    f.seek(SIZE_OF_HASH + SIZE_OF_SALT);

    SecureBytes cryptedData(sizeofMessage);
    SecureBytes data(sizeofMessage);

    f.read(as<char*>(cryptedData), sizeofMessage);
    f.close();

    {
        Crypter crypter;
        MasterDoor door(m_master);
        crypter.cryptData(as<byte*>(data), as<byte*>(cryptedData), sizeofMessage, as<const byte*>(door.get()));
    }

    parseData(data, m_notes, m_master);

    return m_loaded = true;
}

void PassBook::save()
{
    if(!m_changed) {
        return;
    }

    using namespace gost;

    SecureBytes data;

    for(const auto &note : m_notes) {
        data += note.source;
        data += SOURCE_END;
        data += note.URL;
        data += URL_END;
        data += note.login;
        data += LOGIN_END;
        data += note.password.get();
        data += PASS_END;
    }

    const int size = data.size();

    SecureBytes cryptedData(size);

    HashAndSalt hs;
    {
        Crypter crypter;
        MasterDoor door(m_master);
        crypter.cryptData(as<byte*>(cryptedData), as<byte*>(data), size, as<const byte*>(door.get()));
        hs = door.getHash();
    }

    QFile f(m_fileName);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(as<char*>(hs.hash), SIZE_OF_HASH);
    f.write(as<char*>(hs.salt), SIZE_OF_SALT);
    f.write(as<char*>(cryptedData), size);
    f.close();

    m_changed = false;
}

SecureString PassBook::getPassword(int row) const
{
    return m_notes[row].password.get();
}

void PassBook::setPassword(int row, SecureString &&password)
{
    m_notes[row].password.load(std::move(password), m_master);

    QModelIndex idx = index(row, Column::Password);
    emit dataChanged(idx, idx, {Qt::DecorationRole});
    m_changed = true;
}

bool PassBook::noteUp(int i)
{
    if(i < 1) {
        return false;
    }
    m_notes.swap(i, i-1);
    emit dataChanged(index(i, Column::Name), index(i+1, Column::Password), {Qt::DisplayRole, Qt::DecorationRole});
    m_changed = true;

    return true;
}

bool PassBook::noteDown(int i)
{
    if(i >= m_notes.size()-1) {
        return false;
    }
    m_notes.swap(i, i+1);
    emit dataChanged(index(i-1, Column::Name), index(i, Column::Password), {Qt::DisplayRole, Qt::DecorationRole});
    m_changed = true;

    return true;
}

// QAbstractTableModel interface
int PassBook::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_notes.size();
}

int PassBook::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return Column::Count;
}

static const int PIXMAP_W = 150;
static const int PIXMAP_H = 20;

QVariant PassBook::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();
    const Note &note = m_notes[row];

    switch (role) {
        case Qt::EditRole:
        case Qt::DisplayRole: {
            switch (col) {
                case Column::Id: return row+1;
                case Column::Name: return note.source;
                case Column::Url: return note.URL;
                case Column::Login: return note.login;
            case Column::Password: return QVariant::fromValue(note.password);
                default: return QVariant();
            }
        } break;
        case Qt::TextAlignmentRole: {
            switch (col) {
                case Column::Id: return Qt::AlignCenter;
                default: return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
            }
        } break;
        default: return QVariant();
    }
}

QVariant PassBook::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole) {
        return QVariant();
    }

    if(orientation == Qt::Horizontal) {
        switch (section) {
            case Column::Id: return "№";
            case Column::Name: return "Resource";
            case Column::Url: return "URL";
            case Column::Login: return "Login";
            case Column::Password: return "Password";
            default: return QVariant();
        }
    } else {
        return QString::number(section + 1);
    }
}

bool PassBook::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Note &note = m_notes[index.row()];
    QString v = value.toString();

    bool changed = false;

    switch (role) {
        case Qt::EditRole: {
            switch (index.column()) {
                case Column::Name: note.source = v; changed = true; break;
                case Column::Url: note.URL = v; changed = true; break;
                case Column::Login: note.login = v; changed = true; break;
                case Column::Password: note.password = qvariant_cast<Password>(value); changed = true; break;
                default: break;
            }
        } break;
        default: break;
    }

    if(changed) {
        emit dataChanged(index, index, {Qt::DisplayRole});
        m_changed = true;
    }

    return changed;
}

Qt::ItemFlags PassBook::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;

    int c = index.column();
    if(c != Column::Id) {
        f |= Qt::ItemIsEditable;
    }

    return f;
}

bool PassBook::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    for(int i = 0; i<count; ++i) {
        m_notes.insert(row, Note());
    }
    endInsertRows();

    m_changed = true;
    return true;
}

bool PassBook::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    for(int i = 0; i<count; ++i) {
        m_notes.removeAt(row);
    }
    endRemoveRows();

    m_changed = true;
    return true;
}
