#ifndef PASSBOOK_H
#define PASSBOOK_H

#include "securetypes.h"
#include <QVector>
#include <QAbstractTableModel>

struct Note
{
    QString source;
    QString URL;
    QString login;
    Password password;
};

enum_class(Column) {
    Id = 0,
    Name,
    Url,
    Login,
    Password,
    End,
    Count = End
} enum_end;

class PassBook : public QAbstractTableModel {
public:
    PassBook(const QString &fileName, const Master &master);
    int verify(); // returns sizeOfFile OR -1 in case of failure
    bool load();
    void save();
    bool wasChanged() { return m_changed; }
    QList<Note>& notes() { return m_notes; }
    SecureString getPassword(int row) const;
    void setPassword(int row, SecureString &&password);

    bool noteUp(int index);
    bool noteDown(int index);

    // QAbstractTableModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild);
    Qt::DropActions supportedDropActions() const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

private:
    bool m_loaded;
    Master m_master;
    QList<Note> m_notes;
    QString m_fileName;
    bool m_changed;

    void backupFile();
};

#endif //PASSBOOK_H
