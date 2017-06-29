#ifndef KEYEDITDIALOG_H
#define KEYEDITDIALOG_H

#include <QDialog>

namespace Ui {
    class KeyEditDialog;
}

class KeyEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyEditDialog(QString p, QWidget *parent = 0);
    ~KeyEditDialog();

signals:
    void sendKey(QString p);

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::KeyEditDialog *ui;
};

#endif // KEYEDITDIALOG_H
