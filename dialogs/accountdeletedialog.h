#ifndef ACCOUNTDELETEDIALOG_H
#define ACCOUNTDELETEDIALOG_H

#include <QDialog>

namespace Ui {
class AccountDeleteDialog;
}

class AccountDeleteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccountDeleteDialog(const QString &login, QWidget *parent = 0);
    ~AccountDeleteDialog();

signals:
    void accept_deleting();

private:
    Ui::AccountDeleteDialog *ui;
};

#endif // ACCOUNTDELETEDIALOG_H
