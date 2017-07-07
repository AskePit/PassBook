#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>
#include "platform.h"

namespace Ui {
class PasswordDialog;
}

class PasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordDialog(QWidget *parent = 0);
    ~PasswordDialog();

private slots:
    void on_createButton_clicked();
    void on_deleteButton_clicked();
    void on_enterButton_clicked();

    void create_profile(const QString &log, QString &key);
    void delete_profile();

private:
    Ui::PasswordDialog *ui;
};

#endif // PASSWORDDIALOG_H
