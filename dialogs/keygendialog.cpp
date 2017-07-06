#include "keygendialog.h"
#include "ui_keygendialog.h"
#include "utils.h"

KeyGenDialog::KeyGenDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KeyGenDialog)
{
    ui->setupUi(this);
    allignWindowToCenter(this);

    ui->comboBox->addItem("Latin");
    ui->comboBox->addItem("Latin & Numbers");
    ui->comboBox->addItem("General set");
    ui->comboBox->addItem("Expanded set");
    ui->comboBox->setCurrentIndex(3);
}

KeyGenDialog::~KeyGenDialog()
{
    delete ui;
}

void KeyGenDialog::on_pushButton_clicked()
{
    emit sendKeyParams(ui->spinBox->value(), ui->comboBox->currentIndex()+1);
    this->~KeyGenDialog();
}
