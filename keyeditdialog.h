#ifndef KEYEDITDIALOG_H
#define KEYEDITDIALOG_H

#include <QDialog>

namespace Ui {
    class keyEditDialog;
}

class keyEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit keyEditDialog(QString p, QWidget *parent = 0);
    ~keyEditDialog();

signals:
    void sendKey(QString p);

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::keyEditDialog *ui;
};

#endif // KEYEDITDIALOG_H
