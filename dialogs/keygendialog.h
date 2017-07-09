#ifndef KEYGENDIALOG_H
#define KEYGENDIALOG_H

#include <QDialog>

namespace Ui {
class KeyGenDialog;
}

class PassBook;

class KeyGenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyGenDialog(PassBook &passBook, int row, QWidget *parent = 0);
    ~KeyGenDialog();

private:
    Ui::KeyGenDialog *ui;
};

#endif // KEYGENDIALOG_H
