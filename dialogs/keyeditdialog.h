#ifndef KEYEDITDIALOG_H
#define KEYEDITDIALOG_H

#include <QDialog>

class SecureString;

namespace Ui {
    class KeyEditDialog;
}

class KeyEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyEditDialog(const QString &p, QWidget *parent = 0);
    ~KeyEditDialog();

signals:
    void sendKey(QString &p);

private:
    Ui::KeyEditDialog *ui;
};

#endif // KEYEDITDIALOG_H
