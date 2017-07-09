#include "passbookform.h"
#include "ui_passbookform.h"

#include <QClipboard>
#include "utils.h"
#include "passbook.h"

#include "dialogs/passworddialog.h"
#include "dialogs/keygendialog.h"
#include "dialogs/keyeditdialog.h"

PassBookForm::PassBookForm(PassBook* passBook, QString login, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PassBookForm)
    , login(login)
    , passBook(passBook)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    allignWindowToCenter(this);

    ui->passTable->setModel(passBook);

    ui->passTable->setColumnWidth(Column::Id, 35);
    ui->passTable->setColumnWidth(Column::Name, 100);
    ui->passTable->setColumnWidth(Column::Url, 150);
    ui->passTable->setColumnWidth(Column::Login, 100);
    ui->passTable->setColumnWidth(Column::Password, 160);

    addAction(ui->actionSave);

    int maxRow = passBook->notes().size() + 1;
    ui->IdBox->setMaximum(maxRow);
    ui->IdBox->setValue(maxRow);

    enableControls(false);

    connect(ui->passTable, &QTableView::doubleClicked, this, &PassBookForm::doubleClickReact);

    connect(ui->passTable->selectionModel(), &QItemSelectionModel::currentRowChanged, [this](const QModelIndex &current, const QModelIndex &previous) {
        Q_UNUSED(previous);
        enableControls(current.row());
    });

    ui->passTable->setFocus();
}

PassBookForm::~PassBookForm()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
    delete passBook;
    delete ui;
}

static qint32 hashPixmap(const QPixmap& pix)
{
    QImage image = pix.toImage();
    qint32 hash = 0;

    for(int y = 0; y < image.height(); ++y) {
        for(int x = 0; x < image.width(); ++x) {
            QRgb pixel = image.pixel(x,y);

            hash += pixel;
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
    }

    return hash;
}

void PassBookForm::on_addButton_clicked()
{
    int id = ui->IdBox->value()-1;

    passBook->insertRow(id);

    int maxRow = passBook->rowCount()+1;
    ui->IdBox->setMaximum(maxRow);
    ui->IdBox->setValue(maxRow);
}

void PassBookForm::on_deleteButton_clicked()
{
    int row = ui->passTable->currentIndex().row();
    passBook->removeRow(row);
}

void PassBookForm::on_upButton_clicked()
{
    int currentRow = ui->passTable->currentIndex().row();
    bool ok = passBook->noteUp(currentRow);
    if(ok) {
        ui->passTable->selectRow(currentRow-1);
    }
}

void PassBookForm::on_downButton_clicked()
{
    int currentRow = ui->passTable->currentIndex().row();
    bool ok = passBook->noteDown(ui->passTable->currentIndex().row());
    if(ok) {
        ui->passTable->selectRow(currentRow+1);
    }
}

void PassBookForm::save()
{
    passBook->save();
}

void PassBookForm::on_backButton_clicked()
{
    PasswordDialog *w = new PasswordDialog;
    w->show();
    close();
}

void PassBookForm::on_keyGen_clicked()
{
    KeyGenDialog *d = new KeyGenDialog(*passBook, ui->passTable->currentIndex().row());
    d->show();
}

void PassBookForm::doubleClickReact(const QModelIndex& idx)
{
    if(idx.column() == Column::Password) {
        QClipboard *clipboard = QApplication::clipboard();
        SecureString &&pass = passBook->getPassword(idx.row());
        clipboard->setText( QString(std::move(pass)) );
    }
}

void PassBookForm::enableControls(int row)
{
    bool enable = row != -1;

    ui->downButton->setEnabled(enable);
    ui->upButton->setEnabled(enable);
    ui->keyGen->setEnabled(enable);
    ui->keyEdit->setEnabled(enable);

    if(row == 0) {
        ui->upButton->setEnabled(false);
    }

    if(row == passBook->rowCount()-1) {
        ui->downButton->setEnabled(false);
    }
}

void PassBookForm::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
    save();
}

void PassBookForm::on_keyEdit_clicked()
{
    KeyEditDialog *d = new KeyEditDialog(*passBook, ui->passTable->currentIndex().row());
    d->show();
}

void PassBookForm::on_actionSave_triggered()
{
    save();
}
