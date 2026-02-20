#pragma once

#include <QStyledItemDelegate>

class TableEventFilter : public QObject
{
    Q_OBJECT

public:
    bool eventFilter(QObject *watched, QEvent *event);

signals:
    void tableHover(QMouseEvent *event);
    void tableClick(QMouseEvent *event);
    void groupsClick(QMouseEvent *event);
};

class PassBookDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    PassBookDelegate(QWidget *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    bool isInEditMode() { return m_inEditMode; }

public slots:
    void setHoveredPassword(QModelIndex i) { m_hoveredPassword = i; }
    void informDoubleClicked() { m_doubleClicked = true; }

private:
    QModelIndex m_hoveredPassword;
    mutable bool m_inEditMode;
    mutable bool m_doubleClicked;
};
