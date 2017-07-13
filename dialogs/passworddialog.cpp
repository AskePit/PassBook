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

    ui->passwordLine->setFocus();
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
        ui->msgLabel->setText(tr("Account log in error"));
        return;
    }

    Master master(std::move(ui->passwordLine->text()));
    ui->passwordLine->clear();

    PassBook* passBook = new PassBook(filename, master);
    if(!passBook->load()) {
        delete passBook;
        ui->msgLabel->setText(tr("Wrong password"));
        return;
    }

    PassBookForm *passBookForm = new PassBookForm(passBook, ui->loginBox->currentText());
    passBookForm->show();

    close();
}

void PasswordDialog::on_deleteButton_clicked()
{
    ui->msgLabel->clear();
    QString filename(ui->loginBox->currentText() + ACCOUNT_EXT);

    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        ui->msgLabel->setText(tr("Account error"));
        return;
    }

    Master master(std::move(ui->passwordLine->text()));
    ui->passwordLine->clear();

    PassBook passBook(filename, master);
    if(passBook.verify() < 0) {
        ui->msgLabel->setText(tr("Wrong password"));
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

    ui->msgLabel->setText(tr("Account \"%1\" deleted").arg(ui->loginBox->currentText()));
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

    HashAndSalt hs;
    {
        MasterDoor door(master);
        hs = door.getHash();
    }

    QString accountFile = log + ACCOUNT_EXT;
    QFile f(accountFile);
    f.open(QIODevice::WriteOnly);
    f.write(as<char*>(hs.hash), gost::SIZE_OF_HASH);
    f.write(as<char*>(hs.salt), gost::SIZE_OF_SALT);
    f.close();

    PassBook* passBook = new PassBook(accountFile, master);
    PassBookForm *passBookForm = new PassBookForm(passBook, ui->loginBox->currentText());
    passBookForm->show();

    close();
}
