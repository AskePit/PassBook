#include "passbookform.h"
#include "ui_passbookform.h"

#include <QClipboard>
#include <QMouseEvent>
#include <QMenu>
#include "utils.h"
#include "passbook.h"

#include "dialogs/passworddialog.h"
#include "dialogs/keygendialog.h"
#include "dialogs/keyeditdialog.h"

bool TableEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (watched) {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            emit tableHover(qobject_cast<QWidget *>(watched), mouseEvent);
        }
    }

    return QObject::eventFilter(watched, event);
}

PassBookForm::PassBookForm(PassBook* passBook, QString login, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PassBookForm)
    , login(login)
    , passBook(passBook)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
    allignWindowToCenter(this);

    addAction(ui->actionSave);

    enableControls(false);

    TableEventFilter *filter = new TableEventFilter();

    ui->passTable->viewport()->setMouseTracking(true);
    ui->passTable->viewport()->installEventFilter(filter);
    ui->passTable->horizontalHeader()->viewport()->setMouseTracking(true);
    ui->passTable->horizontalHeader()->viewport()->installEventFilter(filter);

    connect(filter, &TableEventFilter::tableHover, [=](QWidget *watched, QMouseEvent *event) {
        int c = ui->passTable->columnAt(event->x());
        bool isTable = (watched == ui->passTable->viewport());
        bool isPassword = (c == Column::Password);

        if(!isTable || !isPassword) {
            passBook->setHoveredPassword(-1);
        } else {
            int r = ui->passTable->rowAt(event->y());
            passBook->setHoveredPassword(r);
        }

        // trigger passwords to be repainted in case of comming from/to horizontal header
        emit passBook->dataChanged(passBook->index(0, Column::Password), passBook->index(passBook->rowCount()-1, Column::Password), {Qt::DecorationRole});
    });

    ui->passTable->setModel(passBook);
    ui->passTable->setColumnWidth(Column::Id, 35);
    ui->passTable->setColumnWidth(Column::Name, 100);
    ui->passTable->setColumnWidth(Column::Url, 150);
    ui->passTable->setColumnWidth(Column::Login, 100);
    ui->passTable->setColumnWidth(Column::Password, 300);
    ui->passTable->horizontalHeader()->setStretchLastSection(true);

    connect(ui->passTable->horizontalHeader(), &QHeaderView::sectionResized, [=](int index, int oldSize, int newSize) {
        Q_UNUSED(oldSize);

        if(index == Column::Password) {
            passBook->setpasswordColumnWidth(newSize);
        }
    });

    connect(ui->passTable->selectionModel(), &QItemSelectionModel::currentRowChanged, [this](const QModelIndex &current, const QModelIndex &previous) {
        Q_UNUSED(previous);
        enableControls(current.row());
    });

    connect(ui->passTable, &QTableView::doubleClicked, this, &PassBookForm::doubleClickReact);
    connect(ui->passTable, &QTableView::customContextMenuRequested, this, &PassBookForm::callPasswordContextMenu);

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
    passBook->insertRow(passBook->rowCount());
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

    if(row == 0) {
        ui->upButton->setEnabled(false);
    }

    if(row == passBook->rowCount()-1) {
        ui->downButton->setEnabled(false);
    }
}

void PassBookForm::callPasswordContextMenu(const QPoint &point)
{
    auto index = ui->passTable->indexAt(point);
    if(!index.isValid()) {
        return;
    }

    QPoint globalPos = ui->passTable->mapToGlobal(point);
    QMenu menu(ui->passTable);
    if(index.column() == Column::Password) {
        menu.addAction(ui->actionEditPassword);
        menu.addAction(ui->actionGeneratePassword);
        menu.addSeparator();
    }
    menu.addAction(ui->actionInsertAbove);
    menu.addAction(ui->actionInsertBelow);

    menu.exec(globalPos);
}

void PassBookForm::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
    save();
}

void PassBookForm::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    int passwordWidth = ui->passTable->columnWidth(Column::Password);
    passBook->setpasswordColumnWidth(passwordWidth);
}

void PassBookForm::on_actionSave_triggered()
{
    save();
}

void PassBookForm::on_actionEditPassword_triggered()
{
    KeyEditDialog *d = new KeyEditDialog(*passBook, ui->passTable->currentIndex().row());
    d->show();
}

void PassBookForm::on_actionGeneratePassword_triggered()
{
    KeyGenDialog *d = new KeyGenDialog(*passBook, ui->passTable->currentIndex().row());
    d->show();
}

void PassBookForm::on_actionInsertAbove_triggered()
{
    passBook->insertRow(ui->passTable->currentIndex().row());
}

void PassBookForm::on_actionInsertBelow_triggered()
{
    passBook->insertRow(ui->passTable->currentIndex().row()+1);
}
