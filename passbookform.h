#ifndef PASSBOOKFORM_H
#define PASSBOOKFORM_H

#include <QWidget>
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
    void tableHover(QWidget *watched, QMouseEvent *event);
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
    void setHoveredPassword(int i) { m_hoveredPassword = i; }
    void informDoubleClicked() { m_doubleClicked = true; }

private:
    int m_hoveredPassword;
    mutable bool m_inEditMode;
    mutable bool m_doubleClicked;
};

class PassBookForm : public QWidget
{
    Q_OBJECT

public:
    explicit PassBookForm(PassBook* passBook, QString login, QWidget *parent = 0);
    ~PassBookForm();

    void closeEvent(QCloseEvent *event);

private slots:
    void on_addButton_clicked();
    void on_deleteButton_clicked();
    void on_backButton_clicked();
    void on_actionSave_triggered();
    void on_actionEditPassword_triggered();
    void on_actionGeneratePassword_triggered();
    void on_actionInsertAbove_triggered();
    void on_actionInsertBelow_triggered();

    void save();
    void doubleClickReact(const QModelIndex& idx);
    void callPasswordContextMenu(const QPoint &pos);

private:
    Ui::PassBookForm *ui;

    QString login;
    PassBook* passBook;
    PassBookDelegate *passBookDelegate;

    bool m_closeWithBack;
    QSettings m_settings;
};

#endif // PASSBOOKFORM_H
