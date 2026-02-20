#pragma once

#include <QDialog>

namespace Ui {
class AccountCreateDialog;
}

class AccountCreateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccountCreateDialog(QWidget *parent = 0);
    ~AccountCreateDialog();

    QString login();
    QString password();

private slots:
    void onButtonBoxRejected();
    void onButtonBoxAccepted();

private:
    Ui::AccountCreateDialog *ui;
};
