#ifndef KEYGENDIALOG_H
#define KEYGENDIALOG_H

#include <QDialog>

namespace Ui {
class KeyGenDialog;
}

class KeyGenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyGenDialog(QWidget *parent = 0);
    ~KeyGenDialog();

signals:
    void sendKeyParams(int n, int mode);

private slots:
    void on_pushButton_clicked();

private:
    Ui::KeyGenDialog *ui;
};

#endif // KEYGENDIALOG_H