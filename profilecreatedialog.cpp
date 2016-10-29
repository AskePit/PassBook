#include "profilecreatedialog.h"
#include "ui_profilecreatedialog.h"

profileCreateDialog::profileCreateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::profileCreateDialog)
{
    ui->setupUi(this);
}

profileCreateDialog::~profileCreateDialog()
{
    delete ui;
}

void profileCreateDialog::on_buttonBox_rejected()
{
    this->~profileCreateDialog();
}

void profileCreateDialog::on_buttonBox_accepted()
{
    if(ui->loginLine->text().isEmpty())                     ui->msg_label->setText("Enter Profile name");
    else if (ui->loginLine->text().size() < 4)              ui->msg_label->setText("Too short profiles name");
    else if (ui->keyLine->text().isEmpty())                 ui->msg_label->setText("Enter password");
    else if (ui->keyLine_2->text().isEmpty())               ui->msg_label->setText("Confirm password");
    else if (ui->keyLine->text() != ui->keyLine_2->text())  ui->msg_label->setText("Passwords do not match");
    else
    {
        emit send_profile_attr(ui->loginLine->text(), ui->keyLine->text());
        this->~profileCreateDialog();
    }
}
