#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>

namespace Ui {
class passwordDialog;
}

class passwordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit passwordDialog(QWidget *parent = 0);
    ~passwordDialog();

signals:
    void ext();     // Выход в главное меню

private slots:

    void on_enterButton_clicked();
    void on_deleteButton_clicked();
    void delete_profile();
    void create_profile(QString log, QString key);
    void on_CreateButton_clicked();

private:
    Ui::passwordDialog *ui;
};

#endif // PASSWORDDIALOG_H
