#include "accountcreatedialog.h"
#include "ui_accountcreatedialog.h"

AccountCreateDialog::AccountCreateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AccountCreateDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog);
    setAttribute(Qt::WA_DeleteOnClose);
}

AccountCreateDialog::~AccountCreateDialog()
{
    delete ui;
}

void AccountCreateDialog::on_buttonBox_rejected()
{
    close();
}

void AccountCreateDialog::on_buttonBox_accepted()
{
    if(ui->loginLine->text().isEmpty())                     ui->msg_label->setText("Enter Login");
    else if (ui->loginLine->text().size() < 4)              ui->msg_label->setText("Too short Login");
    else if (ui->keyLine->text().isEmpty())                 ui->msg_label->setText("Enter password");
    else if (ui->keyLine_2->text().isEmpty())               ui->msg_label->setText("Confirm password");
    else if (ui->keyLine->text() != ui->keyLine_2->text())  ui->msg_label->setText("Passwords do not match");
    else {
        emit sendAccountCredentials(ui->loginLine->text(), ui->keyLine->text());
        close();
    }
}
