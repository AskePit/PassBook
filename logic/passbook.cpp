#include "passbook.h"
#include "crypt.h"
#include "hash.h"
#include "settings.h"
#include "utils.h"
#include <QDir>

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

size_t NotesStorage::getGroupIndex(size_t groupIndex, size_t noteIndex) const
{
    if (groupIndex == NO_GROUPS) {
        return 0;
    } else if (groupIndex == ALL_GROUPS) {
        for (int i = 0; i < m_groupEnds.size(); ++i) {
            if (noteIndex < m_groupEnds[i]) {
                return i;
            }
        }
        return NO_GROUPS;
    } else {
        return groupIndex;
    }
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

size_t NotesStorage::notesCount(size_t groupIndex) const
{
    return getNotes(groupIndex).size();
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
    const bool ok = in.open(QIODevice::ReadOnly);
    if (!ok) {
        return NO_VERSION;
    }

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
    const bool ok = in.open(QIODevice::ReadOnly);
    if (!ok) {
        return -1;
    }

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
    const bool ok = f.open(QIODevice::ReadOnly);
    if (!ok) {
        return m_loaded = false;
    }
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
    const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    if (!ok) {
        return;
    }
    QDataStream out(&f);
    out << VERSION_130;
    out.writeRawData(as<char*>(hs.hash), SIZE_OF_HASH);
    out.writeRawData(as<char*>(hs.salt), SIZE_OF_SALT);
    out.writeRawData(as<char*>(cryptedData), size);

    m_changed = false;
}

SecureString PassBook::getPassword(size_t g, size_t row) const
{
    return m_notes.getNote(g, row).password.get();
}

void PassBook::setPassword(size_t g, size_t row, SecureString &&password)
{
    m_notes.getNote(g, row).password.load(std::move(password), m_master);

    emit passwordChanged(static_cast<int>(row));
    m_changed = true;
}
