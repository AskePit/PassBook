#ifndef KEYGENDIALOG_H
#define KEYGENDIALOG_H

#include <QDialog>

namespace Ui {
class keyGenDialog;
}

class keyGenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit keyGenDialog(QWidget *parent = 0);
    ~keyGenDialog();

signals:
    void sendKeyParams(int n, int mode);

private slots:
    void on_pushButton_clicked();

private:
    Ui::keyGenDialog *ui;
};

#endif // KEYGENDIALOG_H
