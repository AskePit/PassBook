#include "keyeditdialog.h"
#include "ui_keyeditdialog.h"

KeyEditDialog::KeyEditDialog(QString p, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KeyEditDialog)
{
    ui->setupUi(this);
    ui->lineEdit->setText(p);
}

KeyEditDialog::~KeyEditDialog()
{
    delete ui;
}

void KeyEditDialog::on_buttonBox_accepted()
{
    emit sendKey(ui->lineEdit->text());
    this->~KeyEditDialog();
}

void KeyEditDialog::on_buttonBox_rejected()
{
    this->~KeyEditDialog();
}
