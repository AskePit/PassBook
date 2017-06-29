#ifndef PROFILECREATEDIALOG_H
#define PROFILECREATEDIALOG_H

#include <QDialog>

namespace Ui {
class ProfileCreateDialog;
}

class ProfileCreateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileCreateDialog(QWidget *parent = 0);
    ~ProfileCreateDialog();

signals:
    void send_profile_attr(QString log, QString key);

private slots:

    void on_buttonBox_rejected();
    void on_buttonBox_accepted();

private:
    Ui::ProfileCreateDialog *ui;
};

#endif // PROFILECREATEDIALOG_H
