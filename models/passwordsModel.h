#include "logic/passbook.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QTimer>

enum_class(Column) {
    Name = 0,
    Url,
    Login,
    Password,
    End,
    Count = End
} enum_end;

class PasswordsModel : public QAbstractItemModel {
    Q_OBJECT

public:
    PasswordsModel(PassBook& data, QObject *parent = nullptr)
        : QAbstractItemModel(parent)
        , m_data(data)
    {
        QObject::connect(&m_data, &PassBook::passwordChanged, this, [this](int row){
            QModelIndex idx {index(row, Column::Password)};
            emit dataChanged(idx, idx, {Qt::DecorationRole});
        });
    }

    size_t getGroup() const {
        return m_group;
    }

    void setGroup(size_t group) {
        if (group == m_group) {
            return;
        }
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

class PasswordsFilterModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    PasswordsFilterModel(PasswordsModel* sourceModel, QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        setSourceModel(sourceModel);

        m_timer.callOnTimeout([this](){
            doFilterWork();
        });
    }

    void setFilterString(const QString& filterString);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

Q_SIGNALS:
    void filtered();
    void filterCanceled();

private:
    void doFilterWork();
    bool filterPass(QModelIndex currIndex);
    bool isFiltered() const;
    constexpr int getFilterColumn() const;

    QString m_filterString;
    QTimer m_timer;
    QSet<QModelIndex> m_filteredItems;
    QModelIndex m_mostMatched;
};
