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
    explicit KeyGenDialog(PassBook &passBook, int group, int row, QWidget *parent = 0);
    ~KeyGenDialog();

private slots:
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();

private:
    Ui::KeyGenDialog *ui;

    PassBook &m_passBook;
    int m_group;
    int m_row;
};

#endif // KEYGENDIALOG_H
