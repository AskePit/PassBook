#pragma once

#include "securetypes.h"
#include <vector>
#include <span>
#include <algorithm>

#include <QObject>

//! Single password note
struct Note
{
    QString source;
    QString URL;
    QString login;
    Password password;
};

static constexpr size_t ALL_GROUPS = std::numeric_limits<size_t>::max();
static constexpr size_t NO_GROUPS = ALL_GROUPS - 1;

class NotesStorage
{
public:
    std::span<Note> getNotes(const QString& group = QString());
    std::span<Note> getNotes(size_t groupIndex);
    std::span<const Note> getNotes(size_t groupIndex) const;
    const Note& getNote(size_t g, size_t n) const;
    Note& getNote(size_t g, size_t n);

    const std::vector<QString>& getGroups() const;
    size_t getGroupIndex(const QString& group) const;
    // mainly needed to deduce real group in ALL_GROUPS case
    size_t getGroupIndex(size_t groupIndex, size_t noteIndex) const;
    size_t groupsCount() const;
    size_t notesCount(size_t groupIndex = ALL_GROUPS) const;
    bool isEmpty() const;

    const QString& getGroupName(size_t index) const;
    void setGroupName(size_t index, const QString& name);

    void appendGroup(QString&& name, std::vector<Note>&& notes);
    void insertGroup(size_t row, QString&& name);
    void removeGroup(size_t row);
    void moveGroup(size_t src, size_t dst);
    void appendNote(size_t groupIndex, Note&& note);
    void insertNote(size_t groupIndex, size_t noteIndex, Note&& note);
    void removeNote(size_t groupIndex, size_t noteIndex);
    void moveNoteInGroup(size_t groupIndex, size_t src, size_t dst);
    void moveNoteBetweenGroups(size_t srcGroup, size_t srcNote, size_t dstGroup, size_t dstNote = std::numeric_limits<size_t>::max());

private:
    // range is [start; end)
    std::pair<size_t, size_t> getGroupRange(size_t groupIndex) const;
    std::span<Note> getNotesImpl(size_t groupIndex);

    std::vector<Note> m_notes;
    std::vector<size_t> m_groupEnds;
    std::vector<QString> m_groupNames;
};

class NoteId
{
public:
    NoteId() = default;
    NoteId(quintptr id) : m_id{id} {}
    NoteId(int groupIndex) : m_id(groupIndex) {}
    NoteId(int groupIndex, int noteIndex)
        : m_id(0)
    {
        m_id = groupIndex;
        m_id |= (1 << 15);
        m_id |= (noteIndex << 16);
    }

    operator quintptr() { return m_id; }

    bool isNote() { return m_id & (1 << 15); }
    bool isGroup() { return !isNote(); }

    int groupIndex() { return m_id & 0x7FFF; }
    int noteIndex() { return isNote() ? (m_id >> 16) : -1; }

private:
    quintptr m_id {0};
};

enum_class(Column) {
    Name = 0,
    Url,
    Login,
    Password,
    End,
    Count = End
} enum_end;

class PassBook : public QObject {
    Q_OBJECT

    friend class GroupsModel;
    friend class PasswordsModel;

public:
    PassBook(const QString &fileName, const Master &master);
    u32 getDataVersion();
    int verify(); // returns sizeOfFile OR -1 in case of failure
    bool load();
    void save();
    bool wasChanged() { return m_changed; }
    NotesStorage& notes() { return m_notes; }
    SecureString getPassword(size_t g, size_t row) const;
    void setPassword(size_t g, size_t row, SecureString &&password);

signals:
    void passwordChanged(int row);

private:
    bool m_loaded;
    Master m_master;
    NotesStorage m_notes;
    QString m_fileName;
    bool m_changed;

    void backupFile();
};
