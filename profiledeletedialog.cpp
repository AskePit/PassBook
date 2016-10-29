#include "profiledeletedialog.h"
#include "ui_profiledeletedialog.h"

profileDeleteDialog::profileDeleteDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::profileDeleteDialog)
{
    ui->setupUi(this);
}

profileDeleteDialog::~profileDeleteDialog()
{
    delete ui;
}

void profileDeleteDialog::set_login(QString log)
{
    ui->label_2->setText(ui->label_2->text()+"\""+log+"\"?");
}

void profileDeleteDialog::on_buttonBox_accepted()
{
    emit accept_deleting();
    this->~profileDeleteDialog();
}

void profileDeleteDialog::on_buttonBox_rejected()
{
    this->~profileDeleteDialog();
}
