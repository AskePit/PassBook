#include "delegates.h"

#include "logic/securetypes.h"

#include <QTableView>
#include <QMouseEvent>
#include <QPainter>
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

PassBookDelegate::PassBookDelegate(QWidget *parent)
    : QStyledItemDelegate(parent)
    , m_hoveredPassword(QModelIndex())
    , m_inEditMode(false)
    , m_doubleClicked(false)
{}

void PassBookDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Password password { qvariant_cast<Password>(index.data()) };

    const bool show = index == m_hoveredPassword;

    const QRect &rect { option.rect };

    if(option.state & QStyle::State_Selected) {
        painter->fillRect(rect, QColor(0xF5, 0xF5, 0xF5));
    }

    QString pass { password.get() };

    if(pass.isEmpty()) {
        return;
    }

    if(show) {
        QFont font {QStringLiteral("Consolas"), 9};
        QFontMetrics fm {font};
        const int margin {4};
        int w { static_cast<int>(fm.averageCharWidth() * pass.size() + margin) };

        QPixmap pixmap {w, rect.height()};
        pixmap.fill();

        QPainter p(&pixmap);
        p.setFont(font);

        if(option.state & QStyle::State_Selected) {
            p.fillRect(0, 0, rect.width(), rect.height(), QColor(0xF5, 0xF5, 0xF5));
        }
        p.drawText(margin, margin, w, rect.height(), 0, pass);
        painter->drawPixmap(rect.x(), rect.y(), pixmap);
    } else {
        QPixmap pixmap {rect.width(), rect.height()};
        pixmap.fill();

        QPainter p {&pixmap};
        p.fillRect(0, 0, rect.width(), rect.height(), Qt::Dense4Pattern);
        painter->drawPixmap(option.rect.x(), rect.y(), pixmap);
    }
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
