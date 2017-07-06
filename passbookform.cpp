#include "passbookform.h"
#include "ui_passbookform.h"
#include <QLayout>
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
    allignWindowToCenter(this);

    this->show();

    ui->passTable->setColumnWidth(NUMBER_COL, 30);
    ui->passTable->setColumnWidth(RESOURCE_COL, 100);
    ui->passTable->setColumnWidth(URL_COL, 150);
    ui->passTable->setColumnWidth(LOGIN_COL, 100);
    ui->passTable->setColumnWidth(PASSWORD_COL, 150);

    QHBoxLayout *HLayout1 = new QHBoxLayout;
    QHBoxLayout *HLayout2 = new QHBoxLayout;
    QHBoxLayout *HLayout3 = new QHBoxLayout;
    HLayout1->addWidget(ui->passTable);
    HLayout2->addWidget(ui->addButton);
    HLayout2->addWidget(ui->IDBox);
    HLayout2->addStretch(1);
    HLayout2->addWidget(ui->UP);
    HLayout2->addWidget(ui->DOWN);
    HLayout2->addStretch(1);
    HLayout2->addWidget(ui->keyEdit);

    HLayout3->addWidget(ui->deleteButton);
    HLayout3->addStretch(0.1);
    HLayout3->addWidget(ui->SAVE);
    HLayout3->addStretch(0.75);
    HLayout3->addWidget(ui->keyGen);

    QVBoxLayout *VLayout1 = new QVBoxLayout;
    QVBoxLayout *VLayout2 = new QVBoxLayout;
    VLayout1->addWidget(ui->backButton);
    VLayout2->addLayout(HLayout1);
    VLayout2->addLayout(HLayout2);
    VLayout2->addLayout(HLayout3);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setMargin(11);
    mainLayout->setSpacing(6);
    mainLayout->addLayout(VLayout1);
    mainLayout->addLayout(VLayout2);

    connect(ui->passTable,SIGNAL(doubleClicked(QModelIndex)), this, SLOT(doubleClickReact(QModelIndex)));
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

    for(int y = 0; y < image.height(); y++)
    {
        for(int x = 0; x < image.width(); x++)
        {
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
    ui->passTable->setItem(row, PASSWORD_COL, cell);

    Password keeper(std::move(p), master);
    picMap[hashPixmap(pixmap)] = keeper;
}

void PassBookForm::edit_password(QString &p)
{
    renderPasswordPixmap(std::move(p), ui->passTable->currentRow());
}

void PassBookForm::gen_password(int n, int mode)
{
    renderPasswordPixmap(passGenerate(n, mode), ui->passTable->currentRow());
}

void PassBookForm::print_notes()
{
    const QVector<Note>& notes = passBook->getNotes();

    while(ui->passTable->rowCount()) {
        ui->passTable->removeRow(0);
    }

    for(int i = 0; i<notes.size(); ++i)
    {
        const auto &note = notes[i];

        ui->passTable->insertRow(i);
        ui->passTable->setRowHeight(i, 20);

        ui->passTable->setItem(i, NUMBER_COL, new QTableWidgetItem(QString::number(i+1)));
        ui->passTable->setItem(i, RESOURCE_COL, new QTableWidgetItem(note.source));
        ui->passTable->setItem(i, URL_COL, new QTableWidgetItem(note.URL));
        ui->passTable->setItem(i, LOGIN_COL, new QTableWidgetItem(note.login));

        QString &&p = note.password.get(master);

        renderPasswordPixmap(std::move(p), i);

        ui->passTable->item(i, NUMBER_COL)->setTextAlignment(-3);
    }
}


void PassBookForm::sortIDs()
{
    for(int i = 0; i<ui->passTable->rowCount(); i++)
    {
        QTableWidgetItem *Item = new QTableWidgetItem(QString::number(i+1));
        Item->setTextAlignment(-3);
        ui->passTable->setItem(i, NUMBER_COL, Item);
    }
}

void PassBookForm::on_addButton_clicked()
{
    ui->passTable->insertRow(ui->IDBox->value()-1);
    ui->passTable->setRowHeight(ui->IDBox->value()-1, 20);

    ui->passTable->setItem(ui->IDBox->value()-1, RESOURCE_COL, new QTableWidgetItem(""));
    ui->passTable->setItem(ui->IDBox->value()-1, URL_COL, new QTableWidgetItem(""));
    ui->passTable->setItem(ui->IDBox->value()-1, LOGIN_COL, new QTableWidgetItem(""));

    QTableWidgetItem* cell = new QTableWidgetItem();
    cell->setFlags(cell->flags() &= ~Qt::ItemIsEditable);
    ui->passTable->setItem(ui->IDBox->value()-1, PASSWORD_COL, cell);

    ui->IDBox->setMaximum(ui->passTable->rowCount()+1);
    ui->IDBox->setValue(ui->passTable->rowCount()+1);

    sortIDs();
}

void PassBookForm::on_deleteButton_clicked()
{
    ui->passTable->removeRow(ui->passTable->currentRow());
    ui->IDBox->setValue(ui->passTable->rowCount()+1);
    sortIDs();
}

void PassBookForm::on_UP_clicked()
{
    if(ui->passTable->currentRow() <= 0) {
        return;
    }

    int currRowNumber = ui->passTable->currentRow();

    for(int columnIdx = RESOURCE_COL; columnIdx<END_OF_COL; ++columnIdx) {
        QTableWidgetItem* firstItem = ui->passTable->takeItem(currRowNumber-1, columnIdx);
        QTableWidgetItem* secondItem = ui->passTable->takeItem(currRowNumber, columnIdx);

        ui->passTable->setItem(currRowNumber, columnIdx, firstItem);
        ui->passTable->setItem(currRowNumber-1, columnIdx, secondItem);
    }

    ui->passTable->selectRow(currRowNumber-1);
}

void PassBookForm::on_DOWN_clicked()
{
    if(ui->passTable->currentRow() < 0) {
        return;
    }

    if(ui->passTable->currentRow() >= ui->passTable->rowCount()-1) {
        return;
    }

    int currRowNumber = ui->passTable->currentRow();

    for(int columnIdx = RESOURCE_COL; columnIdx<END_OF_COL; ++columnIdx) {
        QTableWidgetItem* firstItem = ui->passTable->takeItem(currRowNumber, columnIdx);
        QTableWidgetItem* secondItem = ui->passTable->takeItem(currRowNumber+1, columnIdx);

        ui->passTable->setItem(currRowNumber+1, columnIdx, firstItem);
        ui->passTable->setItem(currRowNumber, columnIdx, secondItem);
    }

    ui->passTable->selectRow(ui->passTable->currentRow()+1);
}

void PassBookForm::on_SAVE_clicked()
{
    QVector<Note>& notes = passBook->getNotes();
    notes.clear();

    for(int i = 0; i<ui->passTable->rowCount(); ++i)
    {
        struct Note note;
        note.source = ui->passTable->item(i, RESOURCE_COL)->text();
        note.URL    = ui->passTable->item(i, URL_COL)->text();
        note.login  = ui->passTable->item(i, LOGIN_COL)->text();

        QVariant qvar = ui->passTable->item(i, PASSWORD_COL)->data(Qt::DecorationRole);
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
    this->close();
    this->~PassBookForm();
}

void PassBookForm::on_keyGen_clicked()
{
    KeyGenDialog *kG = new KeyGenDialog;

    kG->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
    kG->show();

    QObject::connect(kG, SIGNAL(sendKeyParams(int, int)), this, SLOT(gen_password(int, int)));
}

void PassBookForm::doubleClickReact(const QModelIndex& idx)
{
    if(idx.column() == PASSWORD_COL) {
        QClipboard *clipboard = QApplication::clipboard();

        QVariant qvar = ui->passTable->item(idx.row(), PASSWORD_COL)->data(Qt::DecorationRole);
        QPixmap p = qvar.value<QPixmap>();
        clipboard->setText(picMap[hashPixmap(p)].get(master));
    }
}

void PassBookForm::closeEvent(QCloseEvent *event) {
    (void)event;
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
}

void PassBookForm::on_keyEdit_clicked()
{
    QVariant qvar = ui->passTable->item(ui->passTable->currentRow(), PASSWORD_COL)->data(Qt::DecorationRole);
    QPixmap p = qvar.value<QPixmap>();

    KeyEditDialog *kE = new KeyEditDialog(picMap[hashPixmap(p)].get(master));

    kE->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
    kE->show();

    connect(kE, &KeyEditDialog::sendKey, this, &PassBookForm::edit_password);
}
