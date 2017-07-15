#include "logindialog.h"
#include "ui_logindialog.h"

#include "passbookform.h"
#include "accountdeletedialog.h"
#include "accountcreatedialog.h"
#include "settingsdialog.h"

#include "utils.h"
#include "crypt.h"
#include "hash.h"
#include "passbook.h"

#include <QDir>

static const QString ACCOUNT_EXT{".dat"};

LoginDialog::LoginDialog(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    loadSettings();
    ui->retranslateUi(this);

    loadAccounts();

    connect(ui->passwordLine, &QLineEdit::returnPressed, ui->enterButton, &QPushButton::click);

    ui->passwordLine->setFocus();
    restoreGeometry(iniSettings.value("loginDialogGeometry").toByteArray());
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::closeEvent(QCloseEvent *e)
{
    Q_UNUSED(e);
    iniSettings.setValue("loginDialogGeometry", saveGeometry());
}

void LoginDialog::loadAccounts()
{
    QString &accountsPath {appSettings.accountsPath};
    QString filter(QString{"*%1"}.arg(ACCOUNT_EXT));
    int filterLength {filter.length() - 1};

    QDir accountsDir {accountsPath, filter, QDir::Name, QDir::Files | QDir::Hidden | QDir::NoSymLinks};
    QFileInfoList fileList {accountsDir.entryInfoList()};

    ui->loginBox->clear();
    for(const auto &file : fileList) {
        QString str {file.fileName()};
        str.truncate(str.size() - filterLength);
        ui->loginBox->addItem(str);
    }

    if(ui->loginBox->count() == 0) {
        ui->deleteButton->setDisabled(true);
        ui->passwordLine->setDisabled(true);
        ui->enterButton->setDisabled(true);
    }
}

QString LoginDialog::currentAccountFile()
{
    bool isDefault = appSettings.accountsPath.isEmpty();
    return QString{isDefault ? "%1%2%3": "%1/%2%3"}.arg(appSettings.accountsPath, ui->loginBox->currentText(), ACCOUNT_EXT);
}

void LoginDialog::on_enterButton_clicked()
{
    ui->statusBar->clearMessage();
    QString filename { currentAccountFile() };

    QFile f {filename};
    if (!f.open(QIODevice::ReadOnly)) {
        ui->statusBar->showMessage(tr("Account log in error"));
        return;
    }

    Master master {std::move(ui->passwordLine->text())};
    ui->passwordLine->clear();

    PassBook* passBook = new PassBook {filename, master};
    if(!passBook->load()) {
        delete passBook;
        ui->statusBar->showMessage(tr("Wrong password"));
        return;
    }

    PassBookForm *passBookForm { new PassBookForm {passBook} };
    passBookForm->show();

    close();
}

void LoginDialog::on_deleteButton_clicked()
{
    ui->statusBar->clearMessage();
    QString filename { currentAccountFile() };

    QFile f {filename};
    if (!f.open(QIODevice::ReadOnly)) {
        ui->statusBar->showMessage(tr("Account error"));
        return;
    }

    Master master {std::move(ui->passwordLine->text())};
    ui->passwordLine->clear();

    PassBook passBook {filename, master};
    if(passBook.verify() < 0) {
        ui->statusBar->showMessage(tr("Wrong password"));
        return;
    }

    AccountDeleteDialog *d { new AccountDeleteDialog {ui->loginBox->currentText()} };
    d->show();

    connect(d, &AccountDeleteDialog::accept_deleting, this, &LoginDialog::deleteAccount);
}

void LoginDialog::deleteAccount()
{
    QString filename { currentAccountFile() };
    QFile{filename}.remove();

    ui->statusBar->showMessage(tr("Account \'%1\' deleted").arg(ui->loginBox->currentText()));
    ui->loginBox->removeItem(ui->loginBox->currentIndex());
    ui->passwordLine->clear();

    if(ui->loginBox->count() == 0) {
        ui->deleteButton->setDisabled(true);
        ui->passwordLine->setDisabled(true);
        ui->enterButton->setDisabled(true);
    }
}

void LoginDialog::on_createButton_clicked()
{
    AccountCreateDialog *d { new AccountCreateDialog };
    d->show();

    connect(d, &AccountCreateDialog::sendAccountCredentials, this, &LoginDialog::createAccount);
}

void LoginDialog::createAccount(const QString &log, QString &key)
{
    Master master {std::move(key)};

    HashAndSalt hs;
    {
        MasterDoor door {master};
        hs = door.getHash();
    }

    QString accountFile { QString{"%1/%2%3"}.arg(appSettings.accountsPath, log, ACCOUNT_EXT) };
    QFile f {accountFile};
    f.open(QIODevice::WriteOnly);
    f.write(as<char*>(hs.hash), gost::SIZE_OF_HASH);
    f.write(as<char*>(hs.salt), gost::SIZE_OF_SALT);
    f.close();

    PassBook* passBook { new PassBook{accountFile, master} };
    PassBookForm *passBookForm { new PassBookForm {passBook} };
    passBookForm->show();

    close();
}

void LoginDialog::on_actionAbout_triggered()
{
    QString v = appSettings.version.toString();
    callInfoDialog(tr("Pass Book version %1").arg(v), this);
}

void LoginDialog::on_actionSettings_triggered()
{
    SettingsDialog *d { new SettingsDialog {this} };
    connect(d, &SettingsDialog::languageChanged, [this](){ ui->retranslateUi(this); });
    connect(d, &SettingsDialog::accountsPathChanged, this, &LoginDialog::loadAccounts);
    d->show();
}
