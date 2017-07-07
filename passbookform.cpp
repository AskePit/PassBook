#include "passbookform.h"
#include "ui_passbookform.h"
#include <QSound>
#include <QPainter>
#include <QClipboard>
#include "utils.h"
#include "passbook.h"
#include "crypt.h"
#include "dialogs/passworddialog.h"
#include "dialogs/keygendialog.h"
#include "dialogs/keyeditdialog.h"

PassBookForm::PassBookForm(PassBook* passBook, QString login, const Master &master, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PassBookForm)
    , login(login)
    , master(master)
    , passBook(passBook)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    allignWindowToCenter(this);

    ui->passTable->setColumnWidth(Column::Id, 30);
    ui->passTable->setColumnWidth(Column::Name, 100);
    ui->passTable->setColumnWidth(Column::Url, 150);
    ui->passTable->setColumnWidth(Column::Login, 100);
    ui->passTable->setColumnWidth(Column::Password, 150);

    connect(ui->passTable, &QTableWidget::doubleClicked, this, &PassBookForm::doubleClickReact);
}

PassBookForm::~PassBookForm()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
    delete passBook;
    ui->passTable->clear();
    delete ui;
}

static const int PIXMAP_W = 150;
static const int PIXMAP_H = 50;

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

void PassBookForm::renderPasswordPixmap(QString &&p, int row)
{
    QFont font("Consolas", 10);
    QFontMetrics fm(font);
    int w = fm.width(p);

    QPixmap pixmap(w, PIXMAP_H);
    pixmap.fill();

    QPainter painter(&pixmap);
    painter.setFont(font);
    painter.drawText(0, 17, w, PIXMAP_H, 0, p);

    QTableWidgetItem* cell = new QTableWidgetItem();
    cell->setData(Qt::DecorationRole, pixmap);
    cell->setFlags(cell->flags() &= ~Qt::ItemIsEditable);
    ui->passTable->setItem(row, Column::Password, cell);

    Password keeper(std::move(p), master);
    picMap[hashPixmap(pixmap)] = keeper;
}

void PassBookForm::edit_password(QString &p)
{
    renderPasswordPixmap(std::move(p), ui->passTable->currentRow());
}

void PassBookForm::gen_password(int n, PasswordType::type type)
{
    renderPasswordPixmap(passGenerate(n, type), ui->passTable->currentRow());
}

void PassBookForm::print_notes()
{
    const QVector<Note> &notes = passBook->getNotes();

    while(ui->passTable->rowCount()) {
        ui->passTable->removeRow(0);
    }

    for(int i = 0; i<notes.size(); ++i) {
        const auto &note = notes[i];

        ui->passTable->insertRow(i);
        ui->passTable->setRowHeight(i, 20);

        ui->passTable->setItem(i, Column::Id, new QTableWidgetItem(QString::number(i+1)));
        ui->passTable->setItem(i, Column::Name, new QTableWidgetItem(note.source));
        ui->passTable->setItem(i, Column::Url, new QTableWidgetItem(note.URL));
        ui->passTable->setItem(i, Column::Login, new QTableWidgetItem(note.login));

        QString &&p = note.password.get(master);

        renderPasswordPixmap(std::move(p), i);

        ui->passTable->item(i, Column::Id)->setTextAlignment(-3);
    }
}


void PassBookForm::sortIds()
{
    for(int i = 0; i<ui->passTable->rowCount(); ++i) {
        QTableWidgetItem *Item = new QTableWidgetItem(QString::number(i+1));
        Item->setTextAlignment(-3);
        ui->passTable->setItem(i, Column::Id, Item);
    }
}

void PassBookForm::on_addButton_clicked()
{
    int id = ui->IdBox->value()-1;

    ui->passTable->insertRow(id);
    ui->passTable->setRowHeight(id, 20);

    ui->passTable->setItem(id, Column::Name, new QTableWidgetItem(""));
    ui->passTable->setItem(id, Column::Url, new QTableWidgetItem(""));
    ui->passTable->setItem(id, Column::Login, new QTableWidgetItem(""));

    QTableWidgetItem* cell = new QTableWidgetItem();
    cell->setFlags(cell->flags() &= ~Qt::ItemIsEditable);
    ui->passTable->setItem(id, Column::Password, cell);

    int maxRow = ui->passTable->rowCount()+1;
    ui->IdBox->setMaximum(maxRow);
    ui->IdBox->setValue(maxRow);

    sortIds();
}

void PassBookForm::on_deleteButton_clicked()
{
    ui->passTable->removeRow(ui->passTable->currentRow());
    ui->IdBox->setValue(ui->passTable->rowCount()+1);
    sortIds();
}

void PassBookForm::on_upButton_clicked()
{
    if(ui->passTable->currentRow() <= 0) {
        return;
    }

    int currRowNumber = ui->passTable->currentRow();

    for(int columnIdx = Column::Name; columnIdx < Column::End; ++columnIdx) {
        QTableWidgetItem* firstItem = ui->passTable->takeItem(currRowNumber-1, columnIdx);
        QTableWidgetItem* secondItem = ui->passTable->takeItem(currRowNumber, columnIdx);

        ui->passTable->setItem(currRowNumber, columnIdx, firstItem);
        ui->passTable->setItem(currRowNumber-1, columnIdx, secondItem);
    }

    ui->passTable->selectRow(currRowNumber-1);
}

void PassBookForm::on_downButton_clicked()
{
    if(ui->passTable->currentRow() < 0) {
        return;
    }

    if(ui->passTable->currentRow() >= ui->passTable->rowCount()-1) {
        return;
    }

    int currRowNumber = ui->passTable->currentRow();

    for(int columnIdx = Column::Name; columnIdx < Column::End; ++columnIdx) {
        QTableWidgetItem* firstItem = ui->passTable->takeItem(currRowNumber, columnIdx);
        QTableWidgetItem* secondItem = ui->passTable->takeItem(currRowNumber+1, columnIdx);

        ui->passTable->setItem(currRowNumber+1, columnIdx, firstItem);
        ui->passTable->setItem(currRowNumber, columnIdx, secondItem);
    }

    ui->passTable->selectRow(ui->passTable->currentRow()+1);
}

void PassBookForm::on_saveButton_clicked()
{
    QVector<Note> &notes = passBook->getNotes();
    notes.clear();

    for(int i = 0; i<ui->passTable->rowCount(); ++i) {
        struct Note note;
        note.source = ui->passTable->item(i, Column::Name)->text();
        note.URL    = ui->passTable->item(i, Column::Url)->text();
        note.login  = ui->passTable->item(i, Column::Login)->text();

        QVariant qvar = ui->passTable->item(i, Column::Password)->data(Qt::DecorationRole);
        QPixmap p = qvar.value<QPixmap>();

        note.password = picMap[hashPixmap(p)];
        notes.push_back(note);
    }

    passBook->save(master);

    QSound sound("Okay");
    sound.play();
}

void PassBookForm::on_backButton_clicked()
{
    PasswordDialog *w = new PasswordDialog;
    w->show();
    close();
}

void PassBookForm::on_keyGen_clicked()
{
    KeyGenDialog *d = new KeyGenDialog;
    d->show();

    connect(d, &KeyGenDialog::sendKeyParams, this, &PassBookForm::gen_password);
}

void PassBookForm::doubleClickReact(const QModelIndex& idx)
{
    if(idx.column() == Column::Password) {
        QClipboard *clipboard = QApplication::clipboard();

        QVariant qvar = ui->passTable->item(idx.row(), Column::Password)->data(Qt::DecorationRole);
        QPixmap p = qvar.value<QPixmap>();
        clipboard->setText( QString(picMap[hashPixmap(p)].get(master)) );
    }
}

void PassBookForm::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
    on_saveButton_clicked();
}

void PassBookForm::on_keyEdit_clicked()
{
    QVariant qvar = ui->passTable->item(ui->passTable->currentRow(), Column::Password)->data(Qt::DecorationRole);
    QPixmap p = qvar.value<QPixmap>();

    KeyEditDialog *d = new KeyEditDialog(QString(picMap[hashPixmap(p)].get(master)));
    d->show();

    connect(d, &KeyEditDialog::sendKey, this, &PassBookForm::edit_password);
}
