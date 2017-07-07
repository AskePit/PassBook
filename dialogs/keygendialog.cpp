#include "keygendialog.h"
#include "ui_keygendialog.h"
#include "utils.h"

KeyGenDialog::KeyGenDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::KeyGenDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
    allignWindowToCenter(this);

    for(auto t : PasswordType::iterate()) {
        ui->comboBox->addItem(PasswordType::toString(t));
    }

    ui->comboBox->setCurrentIndex(PasswordType::Standard);
}

KeyGenDialog::~KeyGenDialog()
{
    delete ui;
}

void KeyGenDialog::on_pushButton_clicked()
{
    emit sendKeyParams(ui->spinBox->value(), static_cast<PasswordType::type>(ui->comboBox->currentIndex()));
    close();
}
