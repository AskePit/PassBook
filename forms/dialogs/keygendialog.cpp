#include "keygendialog.h"
#include "ui_keygendialog.h"

#include "logic/passbook.h"
#include "logic/utils.h"

KeyGenDialog::KeyGenDialog(PassBook &passBook, size_t group, size_t row, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::KeyGenDialog)
    , m_passBook(passBook)
    , m_group(group)
    , m_row(row)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &KeyGenDialog::onButtonBoxRejected);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &KeyGenDialog::onButtonBoxAccepted);

    for(auto t : PasswordType::enumerate()) {
        ui->comboBox->addItem(PasswordType::toString(t));
    }

    ui->comboBox->setCurrentIndex(PasswordType::Standard);
}

KeyGenDialog::~KeyGenDialog()
{
    delete ui;
}

void KeyGenDialog::onButtonBoxRejected()
{
    QDialog::reject();
}

void KeyGenDialog::onButtonBoxAccepted()
{
    PasswordType::type type { static_cast<PasswordType::type>(ui->comboBox->currentIndex()) };
    QString p { passGenerate(ui->spinBox->value(), type) };
    m_passBook.setPassword(m_group, m_row, std::move(p));

    QDialog::accept();
}
