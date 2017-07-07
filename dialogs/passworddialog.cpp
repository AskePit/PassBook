#include "passworddialog.h"
#include "ui_passworddialog.h"

#include "passbookform.h"
#include "accountdeletedialog.h"
#include "accountcreatedialog.h"

#include "utils.h"
#include "crypt.h"
#include "hash.h"
#include "passbook.h"

#include <QDir>

static const QString ACCOUNT_EXT(".dat");

PasswordDialog::PasswordDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PasswordDialog)
{
    ui->setupUi(this);
    allignWindowToCenter(this);
    setAttribute(Qt::WA_DeleteOnClose);

    QString accountsPath("");
    QString filter(QString("*%1").arg(ACCOUNT_EXT));
    int filterLength = filter.length() - 1;

    QDir accountsDir(accountsPath, filter, QDir::Name, QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QFileInfoList fileList = accountsDir.entryInfoList();

    for(const auto &file : fileList) {
        QString str(file.fileName());
        str.truncate(str.size() - filterLength);
        ui->loginBox->addItem(str);
    }

    if(ui->loginBox->count() == 0) {
        ui->deleteButton->setDisabled(true);
        ui->passwordLine->setDisabled(true);
        ui->enterButton->setDisabled(true);
    }
}

PasswordDialog::~PasswordDialog()
{
    delete ui;
}

void PasswordDialog::on_enterButton_clicked()
{
    ui->msgLabel->clear();
    QString filename(ui->loginBox->currentText() + ACCOUNT_EXT);

    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        ui->msgLabel->setText("Account log in error");
        return;
    }

    Master master(std::move(ui->passwordLine->text()));
    ui->passwordLine->clear();

    PassBook* passBook = new PassBook(filename);
    if(!passBook->load(master)) {
        delete passBook;
        ui->msgLabel->setText("Wrong password");
        return;
    }

    PassBookForm *passBookForm = new PassBookForm(passBook, ui->loginBox->currentText(), master);
    passBookForm->print_notes();
    passBookForm->show();

    close();
}

void PasswordDialog::on_deleteButton_clicked()
{
    ui->msgLabel->clear();
    QString filename(ui->loginBox->currentText() + ACCOUNT_EXT);

    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        ui->msgLabel->setText("Account error");
        return;
    }

    PassBook passBook(filename);

    Master master(std::move(ui->passwordLine->text()));
    ui->passwordLine->clear();

    if(passBook.verify(master) < 0) {
        ui->msgLabel->setText("Wrong password");
        return;
    }

    AccountDeleteDialog *d = new AccountDeleteDialog(ui->loginBox->currentText());
    d->show();

    connect(d, &AccountDeleteDialog::accept_deleting, this, &PasswordDialog::deleteAccount);
}

void PasswordDialog::deleteAccount()
{
    QString filename = ui->loginBox->currentText() + ACCOUNT_EXT;
    QFile(filename).remove();

    ui->msgLabel->setText("Account \"" + ui->loginBox->currentText()+"\" deleted");
    ui->loginBox->removeItem(ui->loginBox->currentIndex());
    ui->passwordLine->clear();

    if(ui->loginBox->count() == 0) {
        ui->deleteButton->setDisabled(true);
        ui->passwordLine->setDisabled(true);
        ui->enterButton->setDisabled(true);
    }
}

void PasswordDialog::on_createButton_clicked()
{
    AccountCreateDialog *d = new AccountCreateDialog;
    d->show();

    connect(d, &AccountCreateDialog::sendAccountCredentials, this, &PasswordDialog::createAccount);
}

void PasswordDialog::createAccount(const QString &log, QString &key)
{
    Master master(std::move(key));

    QString logFile = log + ACCOUNT_EXT;

    SecureBytes hsh(gost::SIZE_OF_HASH);

    {
        MasterDoor door(master);
        gost::hash(as<byte*>(hsh), door.get(), gost::SIZE_OF_KEY);
    }

    QFile f(logFile);
    f.open(QIODevice::WriteOnly);
    f.write(as<char*>(hsh), gost::SIZE_OF_HASH);
    f.close();

    PassBook* passBook = new PassBook(logFile);
    PassBookForm *passBookForm = new PassBookForm(passBook, ui->loginBox->currentText(), master);
    passBookForm->print_notes();
    passBookForm->show();

    close();
}
