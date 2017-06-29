#include "profiledeletedialog.h"
#include "ui_profiledeletedialog.h"

ProfileDeleteDialog::ProfileDeleteDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProfileDeleteDialog)
{
    ui->setupUi(this);
}

ProfileDeleteDialog::~ProfileDeleteDialog()
{
    delete ui;
}

void ProfileDeleteDialog::set_login(QString log)
{
    ui->label_2->setText(ui->label_2->text()+"\""+log+"\"?");
}

void ProfileDeleteDialog::on_buttonBox_accepted()
{
    emit accept_deleting();
    this->~ProfileDeleteDialog();
}

void ProfileDeleteDialog::on_buttonBox_rejected()
{
    this->~ProfileDeleteDialog();
}
