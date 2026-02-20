#include "accountcreatedialog.h"
#include "ui_accountcreatedialog.h"

AccountCreateDialog::AccountCreateDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AccountCreateDialog)
{
    ui->setupUi(this);
}

AccountCreateDialog::~AccountCreateDialog()
{
    delete ui;
}

void AccountCreateDialog::on_buttonBox_rejected()
{
    QDialog::reject();
}

void AccountCreateDialog::on_buttonBox_accepted()
{
    if(ui->loginLine->text().isEmpty())                     ui->msg_label->setText(tr("Enter Login"));
    else if (ui->keyLine->text().isEmpty())                 ui->msg_label->setText(tr("Enter password"));
    else if (ui->keyLine_2->text().isEmpty())               ui->msg_label->setText(tr("Confirm password"));
    else if (ui->keyLine->text() != ui->keyLine_2->text())  ui->msg_label->setText(tr("Passwords do not match"));
    else {
        QDialog::accept();
    }
}

QString AccountCreateDialog::login()
{
    return ui->loginLine->text();
}

QString AccountCreateDialog::password()
{
    return ui->keyLine->text();
}
