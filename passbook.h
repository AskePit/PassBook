#ifndef PASSBOOK_H
#define PASSBOOK_H

#include "securetypes.h"
#include <QAbstractItemModel>
#include <vector>
#include <span>
#include <algorithm>

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

class noteid
{
public:
    noteid() = default;
    noteid(quintptr id) : m_id{id} {}
    noteid(int groupIndex) : m_id(groupIndex) {}
    noteid(int groupIndex, int noteIndex)
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
    SecureString getPassword(int g, int row) const;
    void setPassword(int g, int row, SecureString &&password);

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

class GroupsModel : public QAbstractItemModel {
    Q_OBJECT

public:
    GroupsModel(PassBook& data)
        : m_data(data)
    {
    }

    QModelIndex groupIndex(const QString &group);
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
private:
    PassBook& m_data;
};

class PasswordsModel : public QAbstractItemModel {
    Q_OBJECT

public:
    PasswordsModel(PassBook& data)
        : m_data(data)
    {
        QObject::connect(&m_data, &PassBook::passwordChanged, this, [this](int row){
            QModelIndex idx {index(row, Column::Password)};
            emit dataChanged(idx, idx, {Qt::DecorationRole});
        });
    }

    void setGroup(size_t group) {
        beginResetModel();
        m_group = group;
        endResetModel();
    }

    void setAllGroups() {
        m_group = ALL_GROUPS;
    }

    void setNoGroups() {
        m_group = NO_GROUPS;
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
private:
    bool isGroupMode() const {
        return m_group != ALL_GROUPS && m_group != NO_GROUPS;
    }

    std::span<Note> getCurrNotes() {
        return m_data.m_notes.getNotes(m_group);
    }

    std::span<Note> getCurrNotes() const {
        return m_data.m_notes.getNotes(m_group);
    }

    Note* getNote(int row) {
        std::span<Note> notes = getCurrNotes();
        return row < notes.size() ? &notes[row] : nullptr;
    }

    const Note* getNote(int row) const {
        std::span<Note> notes = getCurrNotes();
        return row < notes.size() ? &notes[row] : nullptr;
    }

    PassBook& m_data;

    size_t m_group = NO_GROUPS;
};

#endif //PASSBOOK_H
