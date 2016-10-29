#ifndef PROFILEDELETEDIALOG_H
#define PROFILEDELETEDIALOG_H

#include <QDialog>

namespace Ui {
class profileDeleteDialog;
}

class profileDeleteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit profileDeleteDialog(QWidget *parent = 0);
    ~profileDeleteDialog();

    void set_login(QString log);

signals:
    void accept_deleting();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::profileDeleteDialog *ui;
};

#endif // PROFILEDELETEDIALOG_H
