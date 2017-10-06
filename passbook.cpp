#include "passbook.h"
#include "crypt.h"
#include "hash.h"
#include "settings.h"
#include <QDir>
#include <QMimeData>

PassBook::PassBook(const QString &fileName, const Master &master)
    : m_loaded(false)
    , m_master(master)
    , m_fileName(fileName)
    , m_changed(false)
{}

static inline quint64 fileSize(const QString &fileName)
{
    QFileInfo info {fileName};
    return info.size();
}

int PassBook::verify()
{
    using namespace gost;

    quint64 sizeofFile {fileSize(m_fileName)};

    if(sizeofFile < SIZE_OF_HASH + SIZE_OF_SALT) {
        return false;
    }

    SecureBytes fileHash(SIZE_OF_HASH);
    SecureBytes fileSalt(SIZE_OF_SALT);

    QFile in {m_fileName};
    in.open(QIODevice::ReadOnly);
    in.read(as<char *>(fileHash), SIZE_OF_HASH);
    in.read(as<char *>(fileSalt), SIZE_OF_SALT);
    in.close();

    MasterDoor door {m_master};
    SecureBytes realHash (door.getHash(fileSalt));

    return fileHash == realHash ? static_cast<int>(sizeofFile - SIZE_OF_HASH - SIZE_OF_SALT)
                                : -1;
}

static const char SOURCE_END {0x7};
static const char URL_END    {0x8};
static const char LOGIN_END  {0x9};
static const char PASS_END   {0xA};
static const char GROUP_END  {0xB};

static void parseData(const SecureBytes &data, NoteTree &noteTree, const Master &master)
{
    const char* head {data.data()};
    const char* cursor {head};
    const char* end {head + data.size()};
    Note note;
    NoteList noteList;

    while(cursor != end) {
        if(*cursor < SOURCE_END || *cursor > GROUP_END) {
            ++cursor;
            continue;
        }

        byte code = *cursor;

        SecureBytes fieldData(data.mid(head - data.data(), cursor - head));
        QString str {std::move(fieldData)};

        switch(code) {
            case SOURCE_END: note.source = str; break;
            case URL_END:    note.URL    = str; break;
            case LOGIN_END:  note.login  = str; break;
            case PASS_END:   note.password.load(std::move(str), master);
                             noteList.push_back(note);
                             break;
            case GROUP_END:  noteList.setName(str);
                             noteTree.push_back(noteList);
                             noteList.clear();
                             break;
            default: break;
        }

        head = ++cursor;
    }

    if(noteTree.isEmpty()) {
        noteList.setName(QApplication::tr("Common"));
        noteTree.push_back(noteList);
    }
}

bool PassBook::load()
{
    using namespace gost;

    int sizeofMessage {verify()};
    if(sizeofMessage < 0) {
        return m_loaded = false;
    }

    QFile f {m_fileName};
    f.open(QIODevice::ReadOnly);
    f.seek(SIZE_OF_HASH + SIZE_OF_SALT);

    SecureBytes cryptedData(sizeofMessage);
    SecureBytes data(sizeofMessage);

    f.read(as<char*>(cryptedData), sizeofMessage);
    f.close();

    {
        Crypter crypter;
        MasterDoor door {m_master};
        crypter.cryptData(as<byte*>(data), as<byte*>(cryptedData), sizeofMessage, as<const byte*>(door.get()));
    }

    parseData(data, m_notes, m_master);

    return m_loaded = true;
}

void PassBook::backupFile()
{
    QString backupDir { appSettings.accountsPath.isEmpty() ? QStringLiteral("backup") : QString{appSettings.accountsPath + QStringLiteral("/backup")}};
    QDir{}.mkpath(backupDir);
    QString baseName { QFileInfo{m_fileName}.baseName() };
    QString backup1Name { QStringLiteral("%1/%2.backup.1").arg(backupDir, baseName) };
    QString backup2Name { QStringLiteral("%1/%2.backup.2").arg(backupDir, baseName) };
    copyFileForced(backup1Name, backup2Name);
    copyFileForced(m_fileName, backup1Name);
}

void PassBook::save()
{
    if(!m_changed) {
        return;
    }

    backupFile();

    using namespace gost;

    SecureBytes data;

    for(auto &noteList : std::as_const(m_notes)) {
        for(auto &note : std::as_const(noteList)) {
            data += note.source;
            data += SOURCE_END;
            data += note.URL;
            data += URL_END;
            data += note.login;
            data += LOGIN_END;
            data += note.password.get();
            data += PASS_END;
        }
        data += noteList.name();
        data += GROUP_END;
    }

    const int size {data.size()};

    SecureBytes cryptedData(size);

    HashAndSalt hs;
    {
        Crypter crypter;
        MasterDoor door {m_master};
        crypter.cryptData(as<byte*>(cryptedData), as<byte*>(data), size, as<const byte*>(door.get()));
        hs = door.getHash();
    }

    QFile f{m_fileName};
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(as<char*>(hs.hash), SIZE_OF_HASH);
    f.write(as<char*>(hs.salt), SIZE_OF_SALT);
    f.write(as<char*>(cryptedData), size);
    f.close();

    m_changed = false;
}

SecureString PassBook::getPassword(int g, int row) const
{
    return m_notes[g][row].password.get();
}

void PassBook::setPassword(int g, int row, SecureString &&password)
{
    m_notes[g][row].password.load(std::move(password), m_master);

    QModelIndex idx {index(row, Column::Password)};
    emit dataChanged(idx, idx, {Qt::DecorationRole});
    m_changed = true;
}

// QAbstractTableModel interface

#define is_group !parent.isValid()

QModelIndex PassBook::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    if (is_group) {
        return createIndex(row, column, noteid{row});
    } else {
        noteid parentId {parent.internalId()};
        int group = parentId.groupIndex();
        noteid id{group, row};
        return createIndex(row, column, id);
    }
}

QModelIndex PassBook::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    noteid id {index.internalId()};
    if(id.isGroup()) {
        return QModelIndex();
    } else {
        int group = id.groupIndex();
        return createIndex(group, 0, noteid{group});
    }
}

int PassBook::rowCount(const QModelIndex &parent) const
{
    if (is_group) {
        return m_notes.size();
    } else {
        noteid parentId {parent.internalId()};
        if(parentId.isGroup()) {
            int group = parentId.groupIndex();
            return m_notes[group].size();
        } else {
            return 0;
        }
    }
}

int PassBook::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return Column::Count;
}

QVariant PassBook::data(const QModelIndex &index, int role) const
{
    if(role == Qt::SizeHintRole) {
        return QSize(100, 23);
    }

    if(!index.isValid() || (role != Qt::DisplayRole && role != Qt::EditRole)) {
        return QVariant();
    }

    noteid id {index.internalId()};
    int group = id.groupIndex();

    int row {index.row()};
    int col {index.column()};

    if (id.isGroup()) {
        return col == 0 ? m_notes[group].name() : QVariant();
    }

    const Note &note {m_notes[group][row]};

    switch (col) {
        case Column::Name: return note.source;
        case Column::Url: return note.URL;
        case Column::Login: return note.login;
        case Column::Password: return QVariant::fromValue(note.password);
        default: return QVariant();
    }
}

QVariant PassBook::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch (section) {
        case Column::Name: return tr("Resource");
        case Column::Url: return tr("URL");
        case Column::Login: return tr("Login");
        case Column::Password: return tr("Password");
        default: return QVariant();
    }
}

template<class T>
static inline bool setField(T &to, const T &from)
{
    if(to == from) {
        return false;
    } else {
        to = from;
        return true;
    }
}

bool PassBook::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QString v {value.toString()};
    bool changed = false;

    noteid id {index.internalId()};
    int group = id.groupIndex();

    if (id.isGroup()) {
        m_notes[group].setName(v);
        changed = true;
    } else {
        Note &note {m_notes[group][index.row()]};


        switch (role) {
            case Qt::DisplayRole:
            case Qt::EditRole: {
                switch (index.column()) {
                    case Column::Name:     changed |= setField(note.source, v); break;
                    case Column::Url:      changed |= setField(note.URL, v); break;
                    case Column::Login:    changed |= setField(note.login, v); break;
                    case Column::Password: changed |= setField(note.password, qvariant_cast<Password>(value)); break;
                    default: break;
                }
            } break;
            default: break;
        }
    }

    if(changed) {
        emit dataChanged(index, index, {Qt::DisplayRole});
        m_changed = true;
    }

    return changed;
}

bool PassBook::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
{
    if(!roles.contains(Qt::EditRole) && !roles.contains(Qt::DisplayRole)) {
        return false;
    }

    const QVariant &value { roles.contains(Qt::EditRole) ? roles[Qt::EditRole] : roles[Qt::DisplayRole] };
    return setData(index, value);
}

Qt::ItemFlags PassBook::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f { QAbstractItemModel::flags(index) | Qt::ItemIsEnabled };

    noteid id {index.internalId()};
    if(id.isNote() || index.column() == Column::Name) {
        f |=  Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    if(index.isValid()) {
        f |= Qt::ItemIsDragEnabled;
    }

    // drop only in groups and in viewport
    if(!index.isValid() || id.isGroup()) {
        f |= Qt::ItemIsDropEnabled;
    }

    return f;
}

bool PassBook::insertRows(int row, int count, const QModelIndex &parent)
{
    if(row == -1) {
        row = rowCount(parent);
    }

    beginInsertRows(parent, row, row + count - 1);

    if(is_group) {
        for(int i = 0; i<count; ++i) {
            m_notes.insert(row, NoteList{tr("New Group")});
        }
    } else {
        noteid parentId {parent.internalId()};
        if(parentId.isNote()) {
            return false;
        }

        int group = parentId.groupIndex();
        NoteList &noteList = m_notes[group];

        for(int i = 0; i<count; ++i) {
            Note &&note {Note{}};
            note.source = tr("New Password");
            note.password.load(QStringLiteral(""), m_master);
            noteList.insert(row, std::move(note));
        }
    }

    endInsertRows();

    m_changed = true;
    return true;
}

bool PassBook::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    if(is_group) {
        for(int i = 0; i<count; ++i) {
            m_notes.removeAt(row);
        }
    } else {
        noteid parentId {parent.internalId()};
        if(parentId.isNote()) {
            return false;
        }

        int group = parentId.groupIndex();
        NoteList &noteList = m_notes[group];

        for(int i = 0; i<count; ++i) {
            noteList.removeAt(row);
        }
    }

    endRemoveRows();

    m_changed = true;
    return true;
}

Qt::DropActions PassBook::supportedDropActions() const
{
    return Qt::MoveAction;
}

static const QString MIME_TYPE {"application/passbook"};

QStringList PassBook::mimeTypes() const
{
    return { MIME_TYPE };
}

QMimeData *PassBook::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    // Actually all indexes should have same noteid (internalId), because they
    // belong to one dragged row
    if(indexes.size()) {
        QModelIndex index = indexes.first();
        if (index.isValid()) {
            noteid id {index.internalId()};
            stream << static_cast<quintptr>(id);
        }
    }

    mimeData->setData(MIME_TYPE, encodedData);
    return mimeData;
}

bool PassBook::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(action);
    Q_UNUSED(column);

    if (!data->hasFormat(MIME_TYPE)) {
        return false;
    }

    noteid id;
    {
        QByteArray bytes { data->data(MIME_TYPE) };
        QDataStream stream(&bytes, QIODevice::ReadOnly);
        quintptr i;
        stream >> i;
        id = i;
    }

    // drop groups to groups
    if(is_group && id.isNote()) {
        return false;
    }

    // drop notes to notes
    if(!is_group && id.isGroup()) {
        return false;
    }

    // Note: dropMimeData()'s row argument is always ment to be +1 bigger than
    // we expect. Example: If I drag 0 row to row 1, then row argument in this
    // function is 2 because it is ment to create 2nd row, fill it with data
    // and then remove 0 row so that 2nd row becomes the 1st. Further code is
    // written with this fact keeped in mind.

    // insert to 0 means insert to 1st
    if(row == 0) {
        row = 1;
    }

    // insert to -1 means append to the end
    if(row == -1) {
        row = rowCount(parent);
    }

    int srcGroup = id.groupIndex();
    if(id.isNote()) {
        int dstGroup = noteid{parent.internalId()}.groupIndex();
        int srcNote = id.noteIndex();
        int dstNote = row-1;

        if(srcGroup == dstGroup) {
            // move notes within group
            beginMoveRows(parent, srcNote, srcNote, parent, (dstNote == 0 ? dstNote : dstNote+1));
            std::swap(m_notes[srcGroup][srcNote], m_notes[dstGroup][dstNote]);

        } else {
            // move notes among groups
            beginMoveRows(index(srcGroup, 0), srcNote, srcNote, parent, row);
            m_notes[dstGroup].insert(row, m_notes[srcGroup][srcNote]);
            m_notes[srcGroup].removeAt(srcNote);
        }
    } else {
        // move groups
        beginMoveRows(parent, srcGroup, srcGroup, parent, (row == 1 ? row-1 : row));
        std::swap(m_notes[srcGroup], m_notes[row-1]);
    }
    endMoveRows();

    m_changed = true;

    // hack: return false to prevent internal removeRow() function call
    return false;
}
