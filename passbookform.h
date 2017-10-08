#ifndef PASSBOOKFORM_H
#define PASSBOOKFORM_H

#include <QMainWindow>
#include <QStyledItemDelegate>
#include <QSettings>

class PassBook;

namespace Ui {
class PassBookForm;
}

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

class PassBookForm : public QMainWindow
{
    Q_OBJECT

public:
    explicit PassBookForm(PassBook* m_passBook, QWidget *parent = 0);
    ~PassBookForm();

    void closeEvent(QCloseEvent *event);

private slots:
    void on_addPassButton_clicked();
    void on_deletePassButton_clicked();
    void on_addGroupButton_clicked();
    void on_deleteGroupButton_clicked();
    void on_backButton_clicked();
    void on_actionSave_triggered();
    void on_actionEditPassword_triggered();
    void on_actionGeneratePassword_triggered();
    void on_actionInsertPassAbove_triggered();
    void on_actionInsertPassBelow_triggered();
    void on_actionInsertGroupAbove_triggered();
    void on_actionInsertGroupBelow_triggered();

    void save();
    void doubleClickReact(const QModelIndex& idx);
    void checkTableSelection(const QModelIndex& idx);
    void checkListSelection(const QModelIndex& idx);
    void callTableContextMenu(const QPoint &point);
    void callGroupsContextMenu(const QPoint &point);
    void deselectPass();

private:
    Ui::PassBookForm *ui;

    PassBook* m_passBook;
    PassBookDelegate *m_passBookDelegate;

    bool m_closeWithBack;
};

#endif // PASSBOOKFORM_H
