#include "passworddialog.h"
#include "ui_passworddialog.h"

#include "passbookform.h"
#include "profiledeletedialog.h"
#include "profilecreatedialog.h"

#include "utils.h"
#include "crypt.h"
#include "hash.h"
#include "passbook.h"

#include <QDir>

static const QString PROFILE_EXT(".dat");

PasswordDialog::PasswordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PasswordDialog)
{
    ui->setupUi(this);
    allignWindowToCenter(this);
    setAttribute(Qt::WA_DeleteOnClose);

    QString profilesPath("");
    QString filter(QString("*%1").arg(PROFILE_EXT));
    int filterLength = filter.length() - 1;

    QDir profilesDir(profilesPath, filter, QDir::Name, QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QFileInfoList fileList = profilesDir.entryInfoList();

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
    QString filename(ui->loginBox->currentText() + PROFILE_EXT);

    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        ui->msgLabel->setText("Profile log in error");
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
    QString filename(ui->loginBox->currentText() + PROFILE_EXT);

    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        ui->msgLabel->setText("Profile error");
        return;
    }

    PassBook passBook(filename);

    Master master(std::move(ui->passwordLine->text()));
    ui->passwordLine->clear();

    if(passBook.verify(master) < 0) {
        ui->msgLabel->setText("Wrong password");
        return;
    }

    ProfileDeleteDialog *d = new ProfileDeleteDialog;

    d->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog);
    d->set_login(ui->loginBox->currentText());
    d->show();

    connect(d, &ProfileDeleteDialog::accept_deleting, this, &PasswordDialog::delete_profile);
}

void PasswordDialog::delete_profile()
{
    QString filename = ui->loginBox->currentText() + PROFILE_EXT;
    QFile(filename).remove();

    ui->msgLabel->setText("Profile \"" + ui->loginBox->currentText()+"\" deleted");
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
    ProfileCreateDialog *d = new ProfileCreateDialog;

    d->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog);
    d->show();

    connect(d, &ProfileCreateDialog::send_profile_attr, this, &PasswordDialog::create_profile);
}

void PasswordDialog::create_profile(const QString &log, QString &key)
{
    Master master(std::move(key));

    QString logFile = log + PROFILE_EXT;

    byte hsh[gost::SIZE_OF_HASH];

    {
        MasterDoor door(master);
        gost::hash(hsh, door.get(), gost::SIZE_OF_KEY);
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
