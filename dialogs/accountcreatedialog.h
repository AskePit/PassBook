#ifndef ACCOUNTCREATEDIALOG_H
#define ACCOUNTCREATEDIALOG_H

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
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();

private:
    Ui::AccountCreateDialog *ui;
};

#endif // ACCOUNTCREATEDIALOG_H
