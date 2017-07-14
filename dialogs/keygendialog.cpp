#include "keygendialog.h"
#include "ui_keygendialog.h"

#include "passbook.h"
#include "utils.h"
#include <QAbstractButton>

KeyGenDialog::KeyGenDialog(PassBook &passBook, int row, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::KeyGenDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

    for(auto t : PasswordType::iterate()) {
        ui->comboBox->addItem(PasswordType::toString(t));
    }

    ui->comboBox->setCurrentIndex(PasswordType::Standard);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, [&, row] (QAbstractButton *button) {
        bool ok = (QPushButton*)button == ui->buttonBox->button(QDialogButtonBox::Ok);
        if(ok) {
            QString &&p = passGenerate(ui->spinBox->value(), static_cast<PasswordType::type>(ui->comboBox->currentIndex()));
            passBook.setPassword(row, std::move(p));
        }
        close();
    });
}

KeyGenDialog::~KeyGenDialog()
{
    delete ui;
}
