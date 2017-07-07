#include "keyeditdialog.h"
#include "ui_keyeditdialog.h"

#include "securetypes.h"
#include <QAbstractButton>

KeyEditDialog::KeyEditDialog(const QString &p, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::KeyEditDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
    ui->lineEdit->setText(p);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, [this] (QAbstractButton *button) {
        bool ok = (QPushButton*)button == ui->buttonBox->button(QDialogButtonBox::Ok);
        if(ok) {
            emit sendKey(ui->lineEdit->text());
        }
        close();
    });
}

KeyEditDialog::~KeyEditDialog()
{
    delete ui;
}
