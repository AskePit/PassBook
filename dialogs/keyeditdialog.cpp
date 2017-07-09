#include "keyeditdialog.h"
#include "ui_keyeditdialog.h"

#include "passbook.h"
#include <QAbstractButton>

KeyEditDialog::KeyEditDialog(PassBook &passBook, int row, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::KeyEditDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
    ui->lineEdit->setFocus();

    ui->lineEdit->setText(QString(passBook.getPassword(row)));

    connect(ui->buttonBox, &QDialogButtonBox::clicked, [&, row] (QAbstractButton *button) {
        bool ok = (QPushButton*)button == ui->buttonBox->button(QDialogButtonBox::Ok);
        if(ok) {
            passBook.setPassword(row, std::move(ui->lineEdit->text()));
        }
        close();
    });
}

KeyEditDialog::~KeyEditDialog()
{
    delete ui;
}
