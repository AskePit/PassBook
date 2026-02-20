#include "passwordsModel.h"
#include <QMimeData>
#include <QSize>
#include <QIODevice>

QModelIndex PasswordsModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    int group = static_cast<int>(m_data.m_notes.getGroupIndex(m_group, row));
    noteid id{group, row};
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
    return static_cast<int>(
        m_data.m_notes.notesCount(m_group)
    );
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

    const Note* note {getNote(row)};
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

    Note* note {getNote(index.row())};
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
    if (!isGroupMode()) {
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
    if (!isGroupMode()) {
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
    return isGroupMode() ? Qt::MoveAction : Qt::IgnoreAction;
}

static const char* MIME_TYPE {"application/passbook"};

QStringList PasswordsModel::mimeTypes() const
{
    return isGroupMode() ? QStringList{ MIME_TYPE } : QStringList{};
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

bool PasswordsFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!isFiltered()) {
        return true;
    }

    //qDebug() << "filterAcceptsRow";

    auto *model = sourceModel();
    return m_filteredItems.contains(model->index(sourceRow, getFilterColumn(), sourceParent));
}

void PasswordsFilterModel::setFilterString(const QString& filterString) {
    m_filterString = filterString.toLower();
    //qDebug() << "set filter:" << m_filterString;

    m_timer.stop();
    m_timer.start(250);
}

void PasswordsFilterModel::doFilterWork() {
    m_filteredItems.clear();
    m_mostMatched = QModelIndex();

    if(!isFiltered()) {
        beginFilterChange();
        endFilterChange();
        emit filterCanceled();
        return;
    }

    auto *model = sourceModel();
    for(int r = 0; r<model->rowCount(QModelIndex()); ++r) {
        QModelIndex index = model->index(r, getFilterColumn(), QModelIndex());
        if (filterPass(index)) {
            m_filteredItems.insert(index);
        }
    }

    beginFilterChange();
    endFilterChange();
    emit filtered();
}

static int levenshteinDistance(const QString& s1, const QString& s2) {
    const int len1 = static_cast<int>(s1.size()), len2 = static_cast<int>(s2.size());
    std::vector<int> v0(len2 + 1), v1(len2 + 1);

    for (int i = 0; i <= len2; ++i) {
        v0[i] = i;
    }

    for (int i = 0; i < len1; ++i) {
        v1[0] = i + 1;

        for (int j = 0; j < len2; ++j) {
            int cost = (s1[i] == s2[j]) ? 0 : 1;
            v1[j + 1] = std::min({ v1[j] + 1, v0[j + 1] + 1, v0[j] + cost });
        }

        std::swap(v0, v1);
    }

    return v0[len2];
}

bool PasswordsFilterModel::filterPass(QModelIndex currIndex) {
    if (!currIndex.isValid()) {
        return false;
    }

    auto *model = sourceModel();
    const QString& itemString = model->data(currIndex).toString();

    if (m_filterString.size() > 1) {
        if (itemString.startsWith(m_filterString, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return levenshteinDistance(m_filterString, itemString.toLower()) < 3;
}

bool PasswordsFilterModel::isFiltered() const {
    return !m_filterString.isEmpty();
}

constexpr int PasswordsFilterModel::getFilterColumn() const {
    return Column::Name;
}
