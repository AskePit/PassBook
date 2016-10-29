#include "passworddialog.h"
#include "ui_passworddialog.h"

#include "passbookform.h"
#include "profiledeletedialog.h"
#include "profilecreatedialog.h"

#include "instruments.h"
#include "Hash.h"
#include "passbook.h"

#include <QDir>
#include <QTextCodec>

using namespace std;

passwordDialog::passwordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::passwordDialog)
{
    ui->setupUi(this);
    allignWindowToCenter(this);
    //QTextCodec::setCodecForTr(QTextCodec::codecForName("windows-1251") );

    QString profilesPath("");
    QString filter("*.dat");
    size_t filterLength = filter.length() - 1;

    QDir profilesDir(profilesPath, filter, QDir::Name, QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QFileInfoList fileList = profilesDir.entryInfoList();

    for(int i = 0; i<fileList.size(); ++i) {
        QString str = fileList[i].fileName();
        str.remove(str.size() - filterLength, filterLength);
        ui->loginBox->addItem(str);
    }

    if(ui->loginBox->count() == 0) ui->deleteButton->setDisabled(true);
}

passwordDialog::~passwordDialog()
{
    delete ui;
}

void passwordDialog::on_enterButton_clicked()
{
    ui->msgLabel->setText("");
    FILE *prof;
    std::string filename = (this->ui->loginBox->currentText()+".dat").toStdString();

    if ((prof = fopen(filename.c_str(), "rb")) == NULL) {
        ui->msgLabel->setText("Profile log in error");
        fclose(prof);
        return;
    }
    fclose(prof);

    PassBook* passBook = new PassBook(filename);

    byte password[PassBook::SIZEOF_KEY];
    memset(password, 0, PassBook::SIZEOF_KEY);
    memcpy(password, this->ui->keyLine->text().toLatin1(), this->ui->keyLine->text().length());
    ui->keyLine->setText("");

    if(!passBook->load(password)) {
        memset(password, 0, PassBook::SIZEOF_KEY);
        delete passBook;
        ui->msgLabel->setText("Wrong password");
        return;
    }

    PassBookForm *passBookForm = new PassBookForm(passBook, this->ui->loginBox->currentText(), password);
    memset(password, 0, PassBook::SIZEOF_KEY);
    passBookForm->print_notes();
    passBookForm->show();

    this->~passwordDialog();
}

void passwordDialog::on_deleteButton_clicked()
{
    ui->msgLabel->setText("");
    FILE *prof;
    std::string filename = (this->ui->loginBox->currentText()+".dat").toStdString();

    if ((prof = fopen(filename.c_str(), "rb")) == NULL) {
        ui->msgLabel->setText("Profile error");
        fclose(prof);
        return;
    }
    fclose(prof);

    PassBook* passBook = new PassBook(filename);

    byte password[PassBook::SIZEOF_KEY];
    memset(password, 0, PassBook::SIZEOF_KEY);
    memcpy(password, this->ui->keyLine->text().toLatin1(), this->ui->keyLine->text().length());
    ui->keyLine->setText("");

    if(passBook->verify(password) < 0) {
        memset(password, 0, PassBook::SIZEOF_KEY);
        ui->msgLabel->setText("Wrong password");
        return;
    }

    memset(password, 0, PassBook::SIZEOF_KEY);

    profileDeleteDialog *pD = new profileDeleteDialog;

    pD->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog);
    pD->set_login(this->ui->loginBox->currentText());
    pD->show();

    QObject::connect(pD, SIGNAL(accept_deleting()), this, SLOT(delete_profile()));
}

void passwordDialog::delete_profile()
{
    QString filename = this->ui->loginBox->currentText()+".dat";
    unlink(filename.toStdString().c_str());
    ui->msgLabel->setText("Profile \""+this->ui->loginBox->currentText()+"\" deleted");
    ui->loginBox->removeItem(ui->loginBox->currentIndex());
    ui->keyLine->setText("");

    if(ui->loginBox->count() == 0) ui->deleteButton->setDisabled(true);
}

void passwordDialog::on_CreateButton_clicked()
{
    profileCreateDialog *pC = new profileCreateDialog;

    pC->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog);
    pC->show();

    QObject::connect(pC, SIGNAL(send_profile_attr(QString,QString)), this, SLOT(create_profile(QString,QString)));
}

void passwordDialog::create_profile(QString log, QString key)
{
    FILE *f;

    byte keyBuff[PassBook::SIZEOF_KEY];
    memset(keyBuff, 0, PassBook::SIZEOF_KEY);
    memcpy(keyBuff, key.toLatin1(), key.size());

    log += ".dat";
    f = fopen(log.toStdString().c_str(), "wb");

    byte hsh[GOST::SIZEOF_HASH];
    GOST::hash(hsh, keyBuff, PassBook::SIZEOF_KEY);
    fwrite(hsh, GOST::SIZEOF_HASH, 1, f);
    fclose(f);

    PassBook* passBook = new PassBook(log.toStdString());
    PassBookForm *passBookForm = new PassBookForm(passBook, this->ui->loginBox->currentText(), keyBuff);
    passBookForm->print_notes();
    passBookForm->show();

    this->~passwordDialog();
}

