#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>

namespace Ui {
class PasswordDialog;
}

class PasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordDialog(QWidget *parent = 0);
    ~PasswordDialog();

signals:
    void ext();     // ����� � ������� ����

private slots:

    void on_enterButton_clicked();
    void on_deleteButton_clicked();
    void delete_profile();
    void create_profile(QString log, QString key);
    void on_CreateButton_clicked();

private:
    Ui::PasswordDialog *ui;
};

#endif // PASSWORDDIALOG_H
