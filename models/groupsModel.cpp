#include "groupsModel.h"
#include <QSize>
#include <QMimeData>
#include <QIODevice>

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

    return createIndex(row, column, NoteId{row});
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

    NoteId id {index.internalId()};

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
            NoteId id {index.internalId()};
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

    NoteId id;
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
