#include "passbook.h"
#include "crypt.h"
#include "hash.h"
#include "settings.h"
#include <QDir>
#include <QMimeData>

std::span<Note> NotesStorage::getNotes(const QString& group)
{
    if (group.isNull()) {
        return m_notes;
    }

    size_t idx = getGroupIndex(group);
    return getNotes(idx);
}

std::span<Note> NotesStorage::getNotes(size_t groupIndex)
{
    return getNotesImpl(groupIndex);
}

std::span<const Note> NotesStorage::getNotes(size_t groupIndex) const
{
    return const_cast<NotesStorage*>(this)->getNotesImpl(groupIndex);
}

const Note& NotesStorage::getNote(size_t g, size_t n) const
{
    return getNotes(g)[n];
}

Note& NotesStorage::getNote(size_t g, size_t n)
{
    return getNotes(g)[n];
}

size_t NotesStorage::getGroupIndex(const QString& group) const
{
    auto it = std::ranges::find(m_groupNames, group);
    if (it == m_groupNames.end()) {
        return NO_GROUPS;
    }
    return std::distance(m_groupNames.begin(), it);
}

const QString& NotesStorage::getGroupName(size_t index) const
{
    return m_groupNames[index];
}

void NotesStorage::setGroupName(size_t index, const QString& name)
{
    m_groupNames[index] = name;
}

size_t NotesStorage::groupsCount() const
{
    return m_groupNames.size();
}

size_t NotesStorage::notesCount() const
{
    return m_notes.size();
}

bool NotesStorage::isEmpty() const
{
    return m_notes.empty();
}

const std::vector<QString>& NotesStorage::getGroups() const
{
    return m_groupNames;
}

void NotesStorage::appendGroup(QString&& name, std::vector<Note>&& notes)
{
    m_groupNames.emplace_back(std::move(name));
    m_notes.insert(m_notes.end(),
                   std::make_move_iterator(notes.begin()),
                   std::make_move_iterator(notes.end()));

    size_t prevGroupEnd = m_groupEnds.empty() ? 0 : m_groupEnds.back();
    m_groupEnds.push_back(prevGroupEnd + notes.size());
}

void NotesStorage::insertGroup(size_t row, QString&& name)
{
    m_groupNames.insert(m_groupNames.begin() + row, std::move(name));
    const size_t groupEnd = row == 0 ? 0 : m_groupEnds[row - 1];
    m_groupEnds.insert(m_groupEnds.begin() + row, groupEnd);
}

void NotesStorage::removeGroup(size_t row)
{
    auto&& [start, end] = getGroupRange(row);
    const size_t groupSize = end - start;

    m_groupNames.erase(m_groupNames.begin() + row);
    m_groupEnds.erase(m_groupEnds.begin() + row);
    for (size_t i = row; i<m_groupEnds.size(); ++i) {
        m_groupEnds[i] -= groupSize;
    }

    m_notes.erase(m_notes.begin() + start, m_notes.begin() + end);
}

void NotesStorage::moveGroup(size_t src, size_t dst)
{
    if (src == dst || src >= m_groupNames.size() || dst > m_groupNames.size()) {
        return;
    }

    auto [srcStart, srcEnd] = getGroupRange(src);
    const size_t groupSize = srcEnd - srcStart;

    std::vector<Note> groupNotes;
    groupNotes.reserve(groupSize);
    std::move(m_notes.begin() + srcStart, m_notes.begin() + srcEnd, std::back_inserter(groupNotes));

    QString groupName = std::move(m_groupNames[src]);
    removeGroup(src);

    if (dst > src) {
        --dst;
    }

    m_groupNames.insert(m_groupNames.begin() + dst, std::move(groupName));

    size_t insertPos = (dst == 0) ? 0 : m_groupEnds[dst - 1];
    m_notes.insert(m_notes.begin() + insertPos,
                   std::make_move_iterator(groupNotes.begin()),
                   std::make_move_iterator(groupNotes.end()));

    m_groupEnds.insert(m_groupEnds.begin() + dst, insertPos + groupSize);
    for (size_t i = dst + 1; i < m_groupEnds.size(); ++i) {
        m_groupEnds[i] += groupSize;
    }
}

void NotesStorage::appendNote(size_t groupIndex, Note&& note)
{
    auto&& [start, end] = getGroupRange(groupIndex);

    m_notes.insert(m_notes.begin() + end, std::move(note));

    for (size_t i = groupIndex; i < m_groupEnds.size(); ++i) {
        m_groupEnds[i] += 1;
    }
}

void NotesStorage::insertNote(size_t groupIndex, size_t noteIndex, Note&& note)
{
    auto&& [start, end] = getGroupRange(groupIndex);
    const size_t size = end - start;

    if(noteIndex > size) {
        noteIndex = size;
    }

    m_notes.insert(m_notes.begin() + start + noteIndex, std::move(note));

    for (size_t i = groupIndex; i < m_groupEnds.size(); ++i) {
        m_groupEnds[i] += 1;
    }
}

void NotesStorage::removeNote(size_t groupIndex, size_t noteIndex)
{
    auto&& [start, end] = getGroupRange(groupIndex);
    const size_t size = end - start;

    if(noteIndex >= size) {
        noteIndex = size - 1;
    }

    m_notes.erase(m_notes.begin() + start + noteIndex);

    for (size_t i = groupIndex; i < m_groupEnds.size(); ++i) {
        m_groupEnds[i] -= 1;
    }
}

void NotesStorage::moveNoteInGroup(size_t groupIndex, size_t src, size_t dst)
{
    auto&& [start, _] = getGroupRange(groupIndex);

    auto begin = m_notes.begin() + start;
    if (src < dst) {
        std::rotate(begin + src,
                    begin + src + 1,
                    begin + dst + 1);
    } else {
        std::rotate(begin + dst,
                    begin + src,
                    begin + src + 1);
    }
}

void NotesStorage::moveNoteBetweenGroups(size_t srcGroup, size_t srcNote, size_t dstGroup, size_t dstNote)
{

    if (srcGroup >= m_groupNames.size() || dstGroup >= m_groupNames.size()) {
        return; // invalid
    }

    auto [srcStart, srcEnd] = getGroupRange(srcGroup);
    auto [dstStart, dstEnd] = getGroupRange(dstGroup);

    size_t srcSize = srcEnd - srcStart;
    size_t dstSize = dstEnd - dstStart;

    if (srcNote >= srcSize) {
        srcNote = srcSize - 1;
    }

    if (dstNote == std::numeric_limits<size_t>::max()) {
        dstNote = dstSize;
    }
    if (dstNote > dstSize) {
        dstNote = dstSize;
    }

    if (srcGroup == dstGroup) {
        if (srcNote == dstNote) return;

        auto begin = m_notes.begin() + srcStart;
        if (srcNote < dstNote) {
            std::rotate(begin + srcNote,
                        begin + srcNote + 1,
                        begin + dstNote + 1);
        } else {
            std::rotate(begin + dstNote,
                        begin + srcNote,
                        begin + srcNote + 1);
        }
        return;
    }

    Note tmp = std::move(m_notes[srcStart + srcNote]);

    // erase from source
    m_notes.erase(m_notes.begin() + srcStart + srcNote);
    for (size_t i = srcGroup; i < m_groupEnds.size(); ++i) {
        m_groupEnds[i] -= 1;
    }

    // recompute destination range because erase shifted indices
    std::tie(dstStart, dstEnd) = getGroupRange(dstGroup);
    dstSize = dstEnd - dstStart;
    if (dstNote > dstSize) {
        dstNote = dstSize;
    }

    // insert into destination
    m_notes.insert(m_notes.begin() + dstStart + dstNote, std::move(tmp));
    for (size_t i = dstGroup; i < m_groupEnds.size(); ++i) {
        m_groupEnds[i] += 1;
    }
}

// range is [start; end)
std::pair<size_t, size_t> NotesStorage::getGroupRange(size_t groupIndex) const
{
    size_t start = groupIndex == 0 ? 0 : m_groupEnds[groupIndex - 1];
    size_t end = m_groupEnds[groupIndex];

    return {start, end};
}

std::span<Note> NotesStorage::getNotesImpl(size_t groupIndex)
{
    if (groupIndex == NO_GROUPS) {
        return {};
    }
    if (groupIndex == ALL_GROUPS) {
        return m_notes;
    }
    if (groupIndex >= m_groupNames.size()) {
        return {};
    }

    auto&& [start, end] = getGroupRange(groupIndex);
    return { m_notes.begin() + start, m_notes.begin() + end };
}

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

static NotesStorage parseData(const SecureBytes &data, const Master &master, FormatVersion version)
{
    const char* head {data.data()};
    const char* cursor {head};
    const char* end {head + data.size()};
    Note note;
    std::vector<Note> noteList;
    NotesStorage notesStorage;

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
            notesStorage.appendGroup(std::move(str), std::move(noteList));
            noteList.clear();
        }

        head = ++cursor;
    }

    if(notesStorage.isEmpty()) {
        notesStorage.appendGroup(QApplication::tr("Common"), std::move(noteList));
    }

    return notesStorage;
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

    m_notes = parseData(data, m_master, version);

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

    for(const QString& group : m_notes.getGroups()) {
        for(const Note &note : m_notes.getNotes(group)) {
            data += note.source.toUtf8();
            data += SOURCE_END_130;
            data += note.URL.toUtf8();
            data += URL_END_130;
            data += note.login.toUtf8();
            data += LOGIN_END_130;
            data += note.password.get().toUtf8();
            data += PASS_END_130;
        }
        data += group.toUtf8();
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
    return m_notes.getNote(g, row).password.get();
}

void PassBook::setPassword(int g, int row, SecureString &&password)
{
    m_notes.getNote(g, row).password.load(std::move(password), m_master);

    emit passwordChanged(row);
    m_changed = true;
}

QModelIndex GroupsModel::groupIndex(const QString &group)
{
    size_t i = m_data.m_notes.getGroupIndex(group);
    return index(static_cast<int>(i), 0);
}

// QAbstractTableModel interface

QModelIndex GroupsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    return createIndex(row, column, noteid{row});
}

QModelIndex GroupsModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int GroupsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_data.m_notes.groupsCount());
}

int GroupsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant GroupsModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::SizeHintRole) {
        return QSize(100, 23);
    }

    if(!index.isValid() || (role != Qt::DisplayRole && role != Qt::EditRole)) {
        return QVariant();
    }

    int group = index.row();

    return index.column() == 0 ? m_data.m_notes.getGroupName(group) : QVariant();
}

QVariant GroupsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);
    return QVariant();
}

bool GroupsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role);

    QString v {value.toString()};
    int group = index.row();

    m_data.m_notes.setGroupName(group, v);

    emit dataChanged(index, index, {Qt::DisplayRole});
    m_data.m_changed = true;

    return true;
}

bool GroupsModel::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
{
    if(!roles.contains(Qt::EditRole) && !roles.contains(Qt::DisplayRole)) {
        return false;
    }

    const QVariant &value { roles.contains(Qt::EditRole) ? roles[Qt::EditRole] : roles[Qt::DisplayRole] };
    return setData(index, value);
}

Qt::ItemFlags GroupsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f { QAbstractItemModel::flags(index) | Qt::ItemIsEnabled };

    f |=  Qt::ItemIsSelectable | Qt::ItemIsEditable;

    noteid id {index.internalId()};

    if(index.isValid()) {
        f |= Qt::ItemIsDragEnabled;
    }
    f |= Qt::ItemIsDropEnabled;

    return f;
}

bool GroupsModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if(row == -1) {
        row = rowCount(parent);
    }

    beginInsertRows(parent, row, row + count - 1);

    for(int i = 0; i<count; ++i) {
        m_data.m_notes.insertGroup(row, tr("Group"));
    }

    endInsertRows();

    m_data.m_changed = true;
    return true;
}

bool GroupsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    for(int i = 0; i<count; ++i) {
        m_data.m_notes.removeGroup(row);
    }

    endRemoveRows();

    m_data.m_changed = true;
    return true;
}

Qt::DropActions GroupsModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

static const char* MIME_TYPE {"application/passbook"};

QStringList GroupsModel::mimeTypes() const
{
    return { MIME_TYPE };
}

QMimeData *GroupsModel::mimeData(const QModelIndexList &indexes) const
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

bool GroupsModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
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

    // -1 means "drop on th item". and then you look to a parent to see the row number
    if(row == -1) {
        row = parent.row();
    }

    int srcGroup = id.groupIndex();

    // drop notes to groups
    if(id.isNote()) {
        int note = id.noteIndex();
        m_data.m_notes.moveNoteBetweenGroups(srcGroup, note, row, std::numeric_limits<size_t>::max());
        m_data.m_changed = true;
    } else {
        if(row == srcGroup) {
            return false;
        }

        // move groups
        beginMoveRows(parent, srcGroup, srcGroup, parent, row);
        if(row > srcGroup) {
            --row;
        }
        m_data.m_notes.moveGroup(srcGroup, row);
        endMoveRows();

        m_data.m_changed = true;
    }

    // hack: return false to prevent internal removeRow() function call
    return false;
}

// QAbstractTableModel interface

QModelIndex PasswordsModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (m_group < 0) {
        return QModelIndex();
    }

    noteid id{static_cast<int>(m_group), row};
    return createIndex(row, column, id);
}

QModelIndex PasswordsModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int PasswordsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    std::span<Note> notes = GetCurrNotes();
    return static_cast<int>(notes.size());
}

int PasswordsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return Column::Count;
}

QVariant PasswordsModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::SizeHintRole) {
        return QSize(100, 23);
    }

    if(!index.isValid() || (role != Qt::DisplayRole && role != Qt::EditRole)) {
        return QVariant();
    }

    int row {index.row()};
    int col {index.column()};

    const Note* note {GetNote(row)};
    if (!note) {
        return QVariant();
    }

    switch (col) {
        case Column::Name: return note->source;
        case Column::Url: return note->URL;
        case Column::Login: return note->login;
        case Column::Password: return QVariant::fromValue(note->password);
        default: return QVariant();
    }
}

QVariant PasswordsModel::headerData(int section, Qt::Orientation orientation, int role) const
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

bool PasswordsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QString v {value.toString()};
    bool changed = false;

    Note* note {GetNote(index.row())};
    if (!note) {
        return false;
    }

    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            switch (index.column()) {
                case Column::Name:     changed |= setField(note->source, v); break;
                case Column::Url:      changed |= setField(note->URL, v); break;
                case Column::Login:    changed |= setField(note->login, v); break;
                case Column::Password: changed |= setField(note->password, qvariant_cast<Password>(value)); break;
                default: break;
            }
        } break;
        default: break;
    }

    if(changed) {
        emit dataChanged(index, index, {Qt::DisplayRole});
        m_data.m_changed = true;
    }

    return changed;
}

bool PasswordsModel::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
{
    if(!roles.contains(Qt::EditRole) && !roles.contains(Qt::DisplayRole)) {
        return false;
    }

    const QVariant &value { roles.contains(Qt::EditRole) ? roles[Qt::EditRole] : roles[Qt::DisplayRole] };
    return setData(index, value);
}

Qt::ItemFlags PasswordsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f { QAbstractItemModel::flags(index) | Qt::ItemIsEnabled };

    f |=  Qt::ItemIsSelectable | Qt::ItemIsEditable;

    if(index.isValid()) {
        f |= Qt::ItemIsDragEnabled;
    }

    // drop only in groups and in viewport
    if(!index.isValid()) {
        f |= Qt::ItemIsDropEnabled;
    }

    return f;
}

bool PasswordsModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (m_group < 0) {
        return false;
    }

    if(row == -1) {
        row = rowCount(parent);
    }

    beginInsertRows(parent, row, row + count - 1);

    for(int i = 0; i<count; ++i) {
        Note note;
        note.password.load(QStringLiteral(""), m_data.m_master);
        m_data.m_notes.insertNote(m_group, row, std::move(note));
    }

    endInsertRows();

    m_data.m_changed = true;
    return true;
}

bool PasswordsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (m_group < 0) {
        return false;
    }

    beginRemoveRows(parent, row, row + count - 1);

    for(int i = 0; i<count; ++i) {
        m_data.m_notes.removeNote(m_group, row);
    }

    endRemoveRows();

    m_data.m_changed = true;
    return true;
}

Qt::DropActions PasswordsModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList PasswordsModel::mimeTypes() const
{
    return { MIME_TYPE };
}

QMimeData *PasswordsModel::mimeData(const QModelIndexList &indexes) const
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

bool PasswordsModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
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

    // drop groups to notes
    if(id.isGroup()) {
        return false;
    }

    // insert to -1 means append to the end
    if(row == -1) {
        int last = rowCount(parent)-1;
        bool lastNote = id.isNote() && id.noteIndex() == last;
        if(lastNote) {
            return false;
        }

        row = rowCount(parent);
    }

    int srcNote = id.noteIndex();

    if(row == srcNote) {
        return false;
    }

    // move notes within group
    beginMoveRows(parent, srcNote, srcNote, parent, row);
    if(row > srcNote) {
        --row;
    }
    m_data.m_notes.moveNoteInGroup(m_group, srcNote, row);
    endMoveRows();

    m_data.m_changed = true;

    // hack: return false to prevent internal removeRow() function call
    return false;
}
