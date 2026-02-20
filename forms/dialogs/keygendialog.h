#pragma once

#include <QDialog>

namespace Ui {
class KeyGenDialog;
}

class PassBook;

class KeyGenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyGenDialog(PassBook &passBook, size_t group, size_t row, QWidget *parent = 0);
    ~KeyGenDialog();

private slots:
    void onButtonBoxRejected();
    void onButtonBoxAccepted();

private:
    Ui::KeyGenDialog *ui;

    PassBook &m_passBook;
    size_t m_group;
    size_t m_row;
};
