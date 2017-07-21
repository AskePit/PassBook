#include "keygendialog.h"
#include "ui_keygendialog.h"

#include "passbook.h"
#include "utils.h"

KeyGenDialog::KeyGenDialog(PassBook &passBook, int group, int row, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::KeyGenDialog)
    , m_passBook(passBook)
    , m_group(group)
    , m_row(row)
{
    ui->setupUi(this);

    for(auto t : PasswordType::enumerate()) {
        ui->comboBox->addItem(PasswordType::toString(t));
    }

    ui->comboBox->setCurrentIndex(PasswordType::Standard);
}

KeyGenDialog::~KeyGenDialog()
{
    delete ui;
}

void KeyGenDialog::on_buttonBox_rejected()
{
    QDialog::reject();
}

void KeyGenDialog::on_buttonBox_accepted()
{
    PasswordType::type type { static_cast<PasswordType::type>(ui->comboBox->currentIndex()) };
    QString p { passGenerate(ui->spinBox->value(), type) };
    m_passBook.setPassword(m_group, m_row, std::move(p));

    QDialog::accept();
}
