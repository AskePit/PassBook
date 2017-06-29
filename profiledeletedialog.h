#ifndef PROFILEDELETEDIALOG_H
#define PROFILEDELETEDIALOG_H

#include <QDialog>

namespace Ui {
class ProfileDeleteDialog;
}

class ProfileDeleteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileDeleteDialog(QWidget *parent = 0);
    ~ProfileDeleteDialog();

    void set_login(QString log);

signals:
    void accept_deleting();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::ProfileDeleteDialog *ui;
};

#endif // PROFILEDELETEDIALOG_H
