#include "profilecreatedialog.h"
#include "ui_profilecreatedialog.h"

ProfileCreateDialog::ProfileCreateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProfileCreateDialog)
{
    ui->setupUi(this);
}

ProfileCreateDialog::~ProfileCreateDialog()
{
    delete ui;
}

void ProfileCreateDialog::on_buttonBox_rejected()
{
    this->~ProfileCreateDialog();
}

void ProfileCreateDialog::on_buttonBox_accepted()
{
    if(ui->loginLine->text().isEmpty())                     ui->msg_label->setText("Enter Profile name");
    else if (ui->loginLine->text().size() < 4)              ui->msg_label->setText("Too short profiles name");
    else if (ui->keyLine->text().isEmpty())                 ui->msg_label->setText("Enter password");
    else if (ui->keyLine_2->text().isEmpty())               ui->msg_label->setText("Confirm password");
    else if (ui->keyLine->text() != ui->keyLine_2->text())  ui->msg_label->setText("Passwords do not match");
    else
    {
        emit send_profile_attr(ui->loginLine->text(), ui->keyLine->text());
        this->~ProfileCreateDialog();
    }
}
