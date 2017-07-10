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
    QList<Note>& notes() { return m_notes; }
    SecureString getPassword(int row) const;
    void setPassword(int row, SecureString &&password);

    bool noteUp(int index);
    bool noteDown(int index);
    void setHoveredPassword(int i);
    void setpasswordColumnWidth(int width);

    // QAbstractTableModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

private:
    bool m_loaded;
    Master m_master;
    QList<Note> m_notes;
    QString m_fileName;
    bool m_changed;
    int m_hoveredPassword;
    int m_passwordColumnWidth;
};

#endif //PASSBOOK_H
