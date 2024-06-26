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

using FormatVersion = u32;
static constexpr FormatVersion NO_VERSION {std::numeric_limits<u32>::max()};
static constexpr FormatVersion VERSION_130 {0x01030000}; // corresponds to 1.3.0.0 version in big-endian

FormatVersion PassBook::getDataVersion()
{
    QFile in {m_fileName};
    in.open(QIODevice::ReadOnly);

    FormatVersion version;
    QDataStream(in.peek(sizeof(FormatVersion))) >> version;

    // place to adapt to format versions
    if (version != VERSION_130) {
        return NO_VERSION;
    }

    return version;
}

int PassBook::verify()
{
    using namespace gost;

    FormatVersion version = getDataVersion();

    const quint64 sizeofFile {fileSize(m_fileName)};
    const quint64 sizeOfHeader =
            version == NO_VERSION
                ? SIZE_OF_HASH + SIZE_OF_SALT
                : SIZE_OF_HASH + SIZE_OF_SALT + sizeof(FormatVersion);

    if(sizeofFile < sizeOfHeader) {
        return false;
    }

    SecureBytes fileHash(SIZE_OF_HASH);
    SecureBytes fileSalt(SIZE_OF_SALT);

    QFile in {m_fileName};
    in.open(QIODevice::ReadOnly);

    if (version != NO_VERSION) {
        in.seek(sizeof(u32));
    }

    in.read(as<char *>(fileHash), SIZE_OF_HASH);
    in.peek(as<char *>(fileSalt), SIZE_OF_SALT);
    in.close();

    MasterDoor door {m_master};
    SecureBytes realHash (door.getHash(fileSalt));

    return fileHash == realHash
        ? static_cast<int>(sizeofFile - sizeOfHeader)
        : -1;
}

// Those ASCII control characters ARE used by text editors and applications nowadays
// They correspond to \a \b \t \n and \v respectively
// Thus they are NOT SAFE to use for custom text separators
// So, we deprecate this format since 1.3.0
static constexpr char DEPRECATED_SOURCE_END_120 {0x7};
static constexpr char DEPRECATED_URL_END_120    {0x8};
static constexpr char DEPRECATED_LOGIN_END_120  {0x9};
static constexpr char DEPRECATED_PASS_END_120   {0xA};
static constexpr char DEPRECATED_GROUP_END_120  {0xB};

// Those ASCII control characters are NOT used by text editors and applications nowadays
// So, they are safe to use for custom text separators
// We introduce them since 1.3.0
static constexpr char SOURCE_END_130 {0x1};
static constexpr char URL_END_130    {0x2};
static constexpr char LOGIN_END_130  {0x3};
static constexpr char PASS_END_130   {0x4};
static constexpr char GROUP_END_130  {0x5};

static void parseData(const SecureBytes &data, NoteTree &noteTree, const Master &master, FormatVersion version)
{
    const char* head {data.data()};
    const char* cursor {head};
    const char* end {head + data.size()};
    Note note;
    NoteList noteList;

    const char SOURCE_END = version == VERSION_130 ? SOURCE_END_130 : DEPRECATED_SOURCE_END_120;
    const char URL_END    = version == VERSION_130 ? URL_END_130 : DEPRECATED_URL_END_120;
    const char LOGIN_END  = version == VERSION_130 ? LOGIN_END_130 : DEPRECATED_LOGIN_END_120;
    const char PASS_END   = version == VERSION_130 ? PASS_END_130 : DEPRECATED_PASS_END_120;
    const char GROUP_END  = version == VERSION_130 ? GROUP_END_130 : DEPRECATED_GROUP_END_120;


    while(cursor != end) {
        if(*cursor < SOURCE_END || *cursor > GROUP_END) {
            ++cursor;
            continue;
        }

        u8 code = *cursor;

        SecureBytes fieldData(data.mid(head - data.data(), cursor - head));
        QString str {std::move(fieldData)};

        if (code == SOURCE_END) {
            note.source = str;
        } else if (code == URL_END) {
            note.URL = str;
        } else if (code == LOGIN_END) {
            note.login = str;
        } else if (code == PASS_END) {
            note.password.load(std::move(str), master);
            noteList.push_back(note);
        } else if (code == GROUP_END) {
            noteList.setName(str);
            noteTree.push_back(noteList);
            noteList.clear();
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

    FormatVersion version = getDataVersion();

    const quint64 sizeOfHeader =
            version == NO_VERSION
                ? SIZE_OF_HASH + SIZE_OF_SALT
                : SIZE_OF_HASH + SIZE_OF_SALT + sizeof(FormatVersion);

    QFile f {m_fileName};
    f.open(QIODevice::ReadOnly);
    f.seek(sizeOfHeader);

    SecureBytes cryptedData(sizeofMessage);
    SecureBytes data(sizeofMessage);

    f.read(as<char*>(cryptedData), sizeofMessage);
    f.close();

    {
        Crypter crypter;
        MasterDoor door {m_master};
        crypter.cryptData(as<u8*>(data), as<u8*>(cryptedData), sizeofMessage, as<const u8*>(door.get()));
    }

    parseData(data, m_notes, m_master, version);

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

    for(auto &noteList : qAsConst(m_notes)) {
        for(auto &note : qAsConst(noteList)) {
            data += note.source.toUtf8();
            data += SOURCE_END_130;
            data += note.URL.toUtf8();
            data += URL_END_130;
            data += note.login.toUtf8();
            data += LOGIN_END_130;
            data += note.password.get().toUtf8();
            data += PASS_END_130;
        }
        data += noteList.name().toUtf8();
        data += GROUP_END_130;
    }

    const qsizetype size {data.size()};

    SecureBytes cryptedData(size);

    HashAndSalt hs;
    {
        Crypter crypter;
        MasterDoor door {m_master};
        crypter.cryptData(as<u8*>(cryptedData), as<u8*>(data), size, as<const u8*>(door.get()));
        hs = door.getHash();
    }

    QFile f{m_fileName};
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QDataStream out(&f);
    out << VERSION_130;
    out.writeRawData(as<char*>(hs.hash), SIZE_OF_HASH);
    out.writeRawData(as<char*>(hs.salt), SIZE_OF_SALT);
    out.writeRawData(as<char*>(cryptedData), size);

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

QModelIndex PassBook::groupIndex(const QString &group)
{
    int i = m_notes.groupIndex(group);
    return index(i, 0);
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
    if(is_group) {
        return 1;
    }
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
            m_notes.insert(row, NoteList{tr("Group")});
        }
    } else {
        noteid parentId {parent.internalId()};
        if(parentId.isNote()) {
            return false;
        }

        int group = parentId.groupIndex();
        NoteList &noteList = m_notes[group];

        for(int i = 0; i<count; ++i) {
            Note note;
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

    // drop notes to groups
    if(is_group && id.isNote()) {
        return false;
    }

    // drop groups to notes
    if(!is_group && id.isGroup()) {
        return false;
    }

    bool groupsChange = id.isGroup();
    bool notesChange = id.isNote();
    int srcGroup = id.groupIndex();
    int dstGroup = noteid{parent.internalId()}.groupIndex();
    bool sameGroup = !groupsChange && srcGroup == dstGroup;

    // insert to -1 means append to the end
    if(row == -1) {
        if(sameGroup || groupsChange) {
            int last = rowCount(parent)-1;
            bool lastNote = id.isNote() && id.noteIndex() == last;
            bool lastGroup = id.isGroup() && id.groupIndex() == last;
            if(lastNote || lastGroup) {
                return false;
            }
        }

        row = rowCount(parent);
    }

    if(notesChange) {
        int srcNote = id.noteIndex();

        if(sameGroup) {
            if(row == srcNote) {
                return false;
            }

            // move notes within group
            beginMoveRows(parent, srcNote, srcNote, parent, row);
            if(row > srcNote) {
                --row;
            }
            m_notes[dstGroup].move(srcNote, row);
        } else {
            // move notes among groups
            beginMoveRows(index(srcGroup, 0), srcNote, srcNote, parent, row);
            m_notes[dstGroup].insert(row, m_notes[srcGroup][srcNote]);
            m_notes[srcGroup].removeAt(srcNote);
        }
    } else {
        if(row == srcGroup) {
            return false;
        }

        // move groups
        beginMoveRows(parent, srcGroup, srcGroup, parent, row);
        if(row > srcGroup) {
            --row;
        }
        m_notes.move(srcGroup, row);
    }
    endMoveRows();

    m_changed = true;

    // hack: return false to prevent internal removeRow() function call
    return false;
}
