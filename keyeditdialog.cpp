#include "keyeditdialog.h"
#include "ui_keyeditdialog.h"

keyEditDialog::keyEditDialog(QString p, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::keyEditDialog)
{
    ui->setupUi(this);
    ui->lineEdit->setText(p);
}

keyEditDialog::~keyEditDialog()
{
    delete ui;
}

void keyEditDialog::on_buttonBox_accepted()
{
    emit sendKey(ui->lineEdit->text());
    this->~keyEditDialog();
}

void keyEditDialog::on_buttonBox_rejected()
{
    this->~keyEditDialog();
}
