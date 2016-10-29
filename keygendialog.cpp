#include "keygendialog.h"
#include "ui_keygendialog.h"
#include "instruments.h"

keyGenDialog::keyGenDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::keyGenDialog)
{
    ui->setupUi(this);
    allignWindowToCenter(this);

    ui->comboBox->addItem("Latin");
    ui->comboBox->addItem("Latin & Numbers");
    ui->comboBox->addItem("General set");
    ui->comboBox->addItem("Expanded set");
    ui->comboBox->setCurrentIndex(3);
}

keyGenDialog::~keyGenDialog()
{
    delete ui;
}

void keyGenDialog::on_pushButton_clicked()
{
    emit sendKeyParams(ui->spinBox->value(), ui->comboBox->currentIndex()+1);
    this->~keyGenDialog();
}
