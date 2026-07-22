#include "delegates.h"

#include "logic/securetypes.h"

#include <QTableView>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QLineEdit>

bool TableEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    QTableView *table = qobject_cast<QTableView *>( qobject_cast<QWidget *>(watched)->parent() );

    if (table) {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent { static_cast<QMouseEvent *>(event) };
            emit tableHover(mouseEvent);
        } else if(event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent { static_cast<QMouseEvent *>(event) };
            emit tableClick(mouseEvent);
        }
    }

    return QObject::eventFilter(watched, event);
}

QSize FlatItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size { QStyledItemDelegate::sizeHint(option, index) };

    if(m_fixedRowHeight > 0) {
        size.setHeight(m_fixedRowHeight);
    }

    return size;
}

void FlatItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}

PassBookDelegate::PassBookDelegate(QWidget *parent)
    : QStyledItemDelegate(parent)
    , m_hoveredPassword(QModelIndex())
    , m_inEditMode(false)
    , m_doubleClicked(false)
{}

namespace {
const QColor kSelectionColor { 0xE3, 0xE9, 0xF7 };
const QColor kMaskColor { 0x9C, 0xA3, 0xAF };
const QColor kBorderColor { 0xF0, 0xF1, 0xF3 };
constexpr int kMaskDotCount { 8 };
}

void PassBookDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Password password { qvariant_cast<Password>(index.data()) };

    const bool show = index == m_hoveredPassword;

    const QRect &rect { option.rect };

    if(option.state & QStyle::State_Selected) {
        painter->fillRect(rect, kSelectionColor);
    }

    painter->save();
    painter->setPen(QPen(kBorderColor));
    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    painter->restore();

    QString pass { password.get() };

    if(pass.isEmpty()) {
        return;
    }

    painter->save();

    if(show) {
        QFont font {QStringLiteral("Consolas"), 9};
        painter->setFont(font);
        painter->setPen(QPen(QColor(0x1F, 0x23, 0x28)));
        painter->drawText(rect.adjusted(13, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft, pass);
    } else {
        const QString mask { QString(kMaskDotCount, QChar(0x2022)) };
        painter->setPen(QPen(kMaskColor));
        painter->drawText(rect.adjusted(13, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft, mask);
    }

    painter->restore();
}

QWidget *PassBookDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    // do not edit on double click
    if(m_doubleClicked) {
        m_doubleClicked = false;
        return 0;
    }

    QLineEdit *editor { new QLineEdit{parent} };
    return editor;
}

void PassBookDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    m_inEditMode = true;

    QString value = QString{ qvariant_cast<Password>(index.data()).get() };

    QLineEdit *line { static_cast<QLineEdit*>(editor) };
    line->setText(value);
}

void PassBookDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}

void PassBookDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    QLineEdit *line { static_cast<QLineEdit*>(editor) };
    QString value { line->text() };
    Password password { qvariant_cast<Password>(index.data()) };
    password.reload(std::move(value));

    model->setData(index, QVariant::fromValue(password), Qt::EditRole);

    m_inEditMode = false;
}
