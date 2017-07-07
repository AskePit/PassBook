#include "accountdeletedialog.h"
#include "ui_accountdeletedialog.h"

#include <QAbstractButton>

AccountDeleteDialog::AccountDeleteDialog(const QString &login, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AccountDeleteDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->label->setText(QString("Are you sure you want to delete \'%1\'' account?").arg(login));

    connect(ui->buttonBox, &QDialogButtonBox::clicked, [this] (QAbstractButton *button) {
        bool ok = (QPushButton*)button == ui->buttonBox->button(QDialogButtonBox::Ok);
        if(ok) {
            emit accept_deleting();
        }
        close();
    });
}

AccountDeleteDialog::~AccountDeleteDialog()
{
    delete ui;
}
