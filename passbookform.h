#ifndef PASSBOOKFORM_H
#define PASSBOOKFORM_H

#include <QWidget>

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

class PassBookForm : public QWidget
{
    Q_OBJECT

public:
    explicit PassBookForm(PassBook* passBook, QString login, QWidget *parent = 0);
    ~PassBookForm();

    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);

private slots:
    void on_addButton_clicked();
    void on_deleteButton_clicked();
    void on_upButton_clicked();
    void on_downButton_clicked();
    void on_backButton_clicked();
    void on_actionSave_triggered();
    void on_actionEditPassword_triggered();
    void on_actionGeneratePassword_triggered();
    void on_actionInsertAbove_triggered();
    void on_actionInsertBelow_triggered();

    void save();
    void doubleClickReact(const QModelIndex& idx);
    void enableControls(int row);
    void callPasswordContextMenu(const QPoint &pos);

private:
    Ui::PassBookForm *ui;

    QString login;
    PassBook* passBook;
};

#endif // PASSBOOKFORM_H
