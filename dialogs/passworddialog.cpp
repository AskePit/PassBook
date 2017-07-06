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
#include <QTextCodec>

using namespace std;

PasswordDialog::PasswordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PasswordDialog)
{
    ui->setupUi(this);
    allignWindowToCenter(this);

    QString profilesPath("");
    QString filter("*.dat");
    int filterLength = filter.length() - 1;

    QDir profilesDir(profilesPath, filter, QDir::Name, QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QFileInfoList fileList = profilesDir.entryInfoList();

    for(int i = 0; i<fileList.size(); ++i) {
        QString str = fileList[i].fileName();
        str.remove(str.size() - filterLength, filterLength);
        ui->loginBox->addItem(str);
    }

    if(ui->loginBox->count() == 0) ui->deleteButton->setDisabled(true);
}

PasswordDialog::~PasswordDialog()
{
    delete ui;
}

void PasswordDialog::on_enterButton_clicked()
{
    ui->msgLabel->setText("");
    QString filename = (ui->loginBox->currentText()+".dat");

    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        ui->msgLabel->setText("Profile log in error");
        return;
    }

    PassBook* passBook = new PassBook(filename);

    Master master(std::move(ui->keyLine->text()));
    ui->keyLine->setText("");

    if(!passBook->load(master)) {
        delete passBook;
        ui->msgLabel->setText("Wrong password");
        return;
    }

    PassBookForm *passBookForm = new PassBookForm(passBook, ui->loginBox->currentText(), master);
    passBookForm->print_notes();
    passBookForm->show();

    hide();
    //deleteLater();
}

void PasswordDialog::on_deleteButton_clicked()
{
    ui->msgLabel->setText("");
    QString filename = (ui->loginBox->currentText() + ".dat");

    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        ui->msgLabel->setText("Profile error");
        return;
    }

    PassBook* passBook = new PassBook(filename);

    Master master(std::move(ui->keyLine->text()));
    ui->keyLine->setText("");

    if(passBook->verify(master) < 0) {
        ui->msgLabel->setText("Wrong password");
        return;
    }

    ProfileDeleteDialog *pD = new ProfileDeleteDialog;

    pD->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog);
    pD->set_login(ui->loginBox->currentText());
    pD->show();

    QObject::connect(pD, SIGNAL(accept_deleting()), this, SLOT(delete_profile()));
}

void PasswordDialog::delete_profile()
{
    QString filename = ui->loginBox->currentText()+".dat";
    unlink(filename.toStdString().c_str());
    ui->msgLabel->setText("Profile \"" + ui->loginBox->currentText()+"\" deleted");
    ui->loginBox->removeItem(ui->loginBox->currentIndex());
    ui->keyLine->setText("");

    if(ui->loginBox->count() == 0) ui->deleteButton->setDisabled(true);
}

void PasswordDialog::on_createButton_clicked()
{
    ProfileCreateDialog *pC = new ProfileCreateDialog;

    pC->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog);
    pC->show();

    QObject::connect(pC, &ProfileCreateDialog::send_profile_attr, this, &PasswordDialog::create_profile);
}

void PasswordDialog::create_profile(const QString &log, QString &key)
{
    Master master(std::move(key));

    QString logFile = log + ".dat";

    byte hsh[gost::SIZE_OF_HASH];

    {
        MasterDoor door(master);
        gost::hash(hsh, door.get(), gost::SIZE_OF_KEY);
    }

    QFile f(logFile);
    f.open(QIODevice::WriteOnly);
    f.write(reinterpret_cast<char*>(hsh), gost::SIZE_OF_HASH);
    f.close();

    PassBook* passBook = new PassBook(logFile);
    PassBookForm *passBookForm = new PassBookForm(passBook, ui->loginBox->currentText(), master);
    passBookForm->print_notes();
    passBookForm->show();

    this->~PasswordDialog();
}

