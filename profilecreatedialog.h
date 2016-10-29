#ifndef PROFILECREATEDIALOG_H
#define PROFILECREATEDIALOG_H

#include <QDialog>

namespace Ui {
class profileCreateDialog;
}

class profileCreateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit profileCreateDialog(QWidget *parent = 0);
    ~profileCreateDialog();

signals:
    void send_profile_attr(QString log, QString key);

private slots:

    void on_buttonBox_rejected();
    void on_buttonBox_accepted();

private:
    Ui::profileCreateDialog *ui;
};

#endif // PROFILECREATEDIALOG_H
