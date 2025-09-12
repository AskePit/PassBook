#ifndef PASSBOOK_H
#define PASSBOOK_H

#include "securetypes.h"
#include <QAbstractItemModel>

//! Single password note
struct Note
{
    QString source;
    QString URL;
    QString login;
    Password password;
};

//! Named list of password notes (password group)
class NoteList : public QList<Note>
{
public:
    NoteList()
        : QList<Note>()
    {}

    NoteList(const QString &name)
        : QList<Note>()
        , m_name(name)
    {}

    const QString &name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

private:
    QString m_name;
};

//! List of password groups
class NoteTree : public QList<NoteList>
{
public:
    const NoteList *operator[](const QString &group) const {
        for(auto &list : *this) {
            if(list.name() == group) {
                return &list;
            }
        }
        return nullptr;
    }

    NoteList *operator[](const QString &group) {
        for(auto &list : *this) {
            if(list.name() == group) {
                return &list;
            }
        }
        return nullptr;
    }

    int groupIndex(const QString &group) {
        int i = 0;
        for(auto &list : std::as_const(*this)) {
            if(list.name() == group) {
                return i;
            }
            ++i;
        }

        return -1;
    }

    NoteList &operator[](int i) { return QList::operator [](i); }
    const NoteList &operator[](int i) const { return QList::operator [](i); }
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
    NoteTree& notes() { return m_notes; }
    SecureString getPassword(int g, int row) const;
    void setPassword(int g, int row, SecureString &&password);

signals:
    void passwordChanged(int row);

private:
    bool m_loaded;
    Master m_master;
    NoteTree m_notes;
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

    void setGroup(int group) {
        m_group = group;
    }

    void setAllGroups() {
        m_group = ALL_GROUPS;
    }

    void setNoGroups() {
        m_group = NO_GROUPS;
    }

    void invalidate() {
        emit dataChanged(index(0, 0), index(GetCurrNotes().size(), Column::Count));
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
    NoteList& GetCurrNotes() {
        return m_data.m_notes[m_group];
    }

    const NoteList& GetCurrNotes() const {
        return m_data.m_notes[m_group];
    }

    Note& GetNote(int row) {
        return GetCurrNotes()[row];
    }

    const Note& GetNote(int row) const {
        return GetCurrNotes()[row];
    }

    PassBook& m_data;

    static constexpr int NO_GROUPS = -2;
    static constexpr int ALL_GROUPS = -1;
    int m_group = NO_GROUPS;
};

#endif //PASSBOOK_H
