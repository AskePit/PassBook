#include "logindialog.h"
#include "ui_logindialog.h"

#include "passbookform.h"
#include "accountcreatedialog.h"
#include "settingsdialog.h"

#include "utils.h"
#include "Crypt.h"
#include "Hash.h"
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
    restoreGeometry(iniSettings.value(QStringLiteral("LoginDialogGeometry")).toByteArray());
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::closeEvent(QCloseEvent *e)
{
    Q_UNUSED(e);
    iniSettings.setValue(QStringLiteral("LoginDialogGeometry"), saveGeometry());
    iniSettings.setValue(QStringLiteral("LastAccount"), ui->loginBox->currentText());
}

void LoginDialog::loadAccounts()
{
    QString &accountsPath {appSettings.accountsPath};
    QString filter(QStringLiteral("*%1").arg(ACCOUNT_EXT));
    qsizetype filterLength {filter.length() - 1};

    QDir accountsDir {accountsPath, filter, QDir::Name, QDir::Files | QDir::Hidden | QDir::NoSymLinks};
    const QFileInfoList fileList {accountsDir.entryInfoList()};

    ui->loginBox->clear();
    for(auto &file : fileList) {
        QString str {file.fileName()};
        str.truncate(str.size() - filterLength);
        ui->loginBox->addItem(str);
    }

    bool accountsAvaliable = ui->loginBox->count() != 0;
    ui->deleteButton->setEnabled(accountsAvaliable);
    ui->passwordLine->setEnabled(accountsAvaliable);
    ui->enterButton->setEnabled(accountsAvaliable);

    ui->loginBox->setCurrentText( iniSettings.value(QStringLiteral("LastAccount")).toString() );
}

QString LoginDialog::currentAccountFile(QString newLogin)
{
    bool isDefault = appSettings.accountsPath.isEmpty();
    QString login { newLogin.isNull() ? ui->loginBox->currentText() : newLogin };
    return QString(isDefault ? QStringLiteral("%1%2%3"): QStringLiteral("%1/%2%3")).arg(appSettings.accountsPath, login, ACCOUNT_EXT);
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

    Master master {ui->passwordLine->text()};
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
    f.close();

    Master master {ui->passwordLine->text()};
    ui->passwordLine->clear();

    PassBook passBook {filename, master};
    if(passBook.verify() < 0) {
        ui->statusBar->showMessage(tr("Wrong password"));
        return;
    }

    int res = callQuestionDialog(
                  tr("Are you sure you want to delete \'%1\' account?")
                    .arg(ui->loginBox->currentText()),
                  this
    );

    if(res == QMessageBox::Ok) {
        deleteAccount();
    }
}

void LoginDialog::deleteAccount()
{
    QFile::remove(currentAccountFile());

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
    AccountCreateDialog d { this };
    int res = d.exec();

    if(res == QDialog::Accepted) {
        createAccount(d.login(), d.password());
    }
}

void LoginDialog::createAccount(const QString &log, QString &&key)
{
    Master master {std::move(key)};

    HashAndSalt hs;
    {
        MasterDoor door {master};
        hs = door.getHash();
    }

    QString accountFile { currentAccountFile(log) };
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
