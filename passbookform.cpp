#include "passbookform.h"
#include "ui_passbookform.h"

#include <QClipboard>
#include <QMouseEvent>
#include <QMenu>
#include <QLineEdit>
#include "utils.h"
#include "passbook.h"

#include "dialogs/logindialog.h"
#include "dialogs/keygendialog.h"

bool TableEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (watched) {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent { static_cast<QMouseEvent *>(event) };
            emit tableHover(qobject_cast<QWidget *>(watched), mouseEvent);
        }
    }

    return QObject::eventFilter(watched, event);
}

PassBookDelegate::PassBookDelegate(QWidget *parent)
    : QStyledItemDelegate(parent)
    , m_hoveredPassword(-1)
    , m_inEditMode(false)
    , m_doubleClicked(false)
{}

void PassBookDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Password password { qvariant_cast<Password>(index.data()) };
    password.paint(painter, option, index.row() == m_hoveredPassword);
}

QWidget *PassBookDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    // do not edit on double click
    if(m_doubleClicked) {
        m_doubleClicked = false;
        return 0;
    }

    QLineEdit *editor { new QLineEdit{parent} };
    return editor;
}

void PassBookDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    m_inEditMode = true;

    QString value = QString{ qvariant_cast<Password>(index.data()).get() };

    QLineEdit *line { static_cast<QLineEdit*>(editor) };
    line->setText(value);
}

void PassBookDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    QLineEdit *line { static_cast<QLineEdit*>(editor) };
    QString value { line->text() };
    Password password { qvariant_cast<Password>(index.data()) };
    password.reload(std::move(value));

    model->setData(index, QVariant::fromValue(password), Qt::EditRole);

    m_inEditMode = false;
}

PassBookForm::PassBookForm(PassBook* passBook, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PassBookForm)
    , m_passBook(passBook)
    , m_passBookDelegate(new PassBookDelegate)
    , m_closeWithBack(false)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);

    addAction(ui->actionSave);

    TableEventFilter *filter { new TableEventFilter };

    ui->passTable->viewport()->setMouseTracking(true);
    ui->passTable->viewport()->installEventFilter(filter);
    ui->passTable->horizontalHeader()->viewport()->setMouseTracking(true);
    ui->passTable->horizontalHeader()->viewport()->installEventFilter(filter);

    ui->passTable->setModel(passBook);
    ui->passTable->setItemDelegateForColumn(Column::Password, m_passBookDelegate);
    ui->passTable->setColumnWidth(Column::Id, 35);
    ui->passTable->setColumnWidth(Column::Name, 100);
    ui->passTable->setColumnWidth(Column::Url, 150);
    ui->passTable->setColumnWidth(Column::Login, 100);
    ui->passTable->setColumnWidth(Column::Password, 300);
    ui->passTable->horizontalHeader()->setStretchLastSection(true);

    connect(ui->passTable, &QTableView::doubleClicked, this, &PassBookForm::doubleClickReact);
    connect(ui->passTable, &QTableView::customContextMenuRequested, this, &PassBookForm::callPasswordContextMenu);

    ui->passTable->setFocus();

    connect(filter, &TableEventFilter::tableHover, [=](QWidget *watched, QMouseEvent *event) {        
        int c { ui->passTable->columnAt(event->x()) };
        bool isTable { watched == ui->passTable->viewport() };
        bool isPassword { c == Column::Password };

        if(m_passBookDelegate->isInEditMode()) {
            return;
        }

        if(!isTable || !isPassword) {
            m_passBookDelegate->setHoveredPassword(-1);
        } else {
            int r = ui->passTable->rowAt(event->y());
            m_passBookDelegate->setHoveredPassword(r);
        }

        // trigger passwords to be repainted in case of comming from/to horizontal header
        emit passBook->dataChanged(passBook->index(0, Column::Password), passBook->index(passBook->rowCount()-1, Column::Password), {Qt::DisplayRole});
    });

    ui->passTable->viewport()->setAcceptDrops(true);
    restoreGeometry(iniSettings.value("mainFormGeometry").toByteArray());
}

PassBookForm::~PassBookForm()
{
    QClipboard *clipboard { QApplication::clipboard() };
    clipboard->clear();
    delete m_passBook;
    delete ui;
}

static qint32 hashPixmap(const QPixmap& pix)
{
    QImage image { pix.toImage() };
    qint32 hash {0};

    for(int y = 0; y < image.height(); ++y) {
        for(int x = 0; x < image.width(); ++x) {
            QRgb pixel { image.pixel(x,y) };

            hash += pixel;
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
    }

    return hash;
}

void PassBookForm::on_addButton_clicked()
{
    m_passBook->insertRow(m_passBook->rowCount());
}

void PassBookForm::on_deleteButton_clicked()
{
    int row { ui->passTable->currentIndex().row() };
    m_passBook->removeRow(row);
}

void PassBookForm::save()
{
    m_passBook->save();
}

void PassBookForm::on_backButton_clicked()
{
    m_closeWithBack = true;
    close();
}

void PassBookForm::doubleClickReact(const QModelIndex& idx)
{
    if(idx.column() == Column::Password) {
        m_passBookDelegate->informDoubleClicked();
        QClipboard *clipboard { QApplication::clipboard() };
        SecureString &&pass { m_passBook->getPassword(idx.row()) };
        clipboard->setText( QString{std::move(pass)} );
    }
}

void PassBookForm::callPasswordContextMenu(const QPoint &point)
{
    QModelIndex index { ui->passTable->indexAt(point) };
    if(!index.isValid()) {
        return;
    }

    QPoint globalPos { ui->passTable->mapToGlobal(point) };
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
    QClipboard *clipboard { QApplication::clipboard() };
    clipboard->clear();

    iniSettings.setValue("mainFormGeometry", saveGeometry());

    if(m_passBook->wasChanged()) {
        int ret { callQuestionDialog(tr("Do you want to save changes?"), this) };
        if(ret == QMessageBox::Ok) {
            save();
        }
    }

    if(m_closeWithBack) {
        LoginDialog *w = new LoginDialog;
        w->show();
    }
}

void PassBookForm::on_actionSave_triggered()
{
    save();
}

void PassBookForm::on_actionEditPassword_triggered()
{
    ui->passTable->edit(ui->passTable->currentIndex());
}

void PassBookForm::on_actionGeneratePassword_triggered()
{
    KeyGenDialog *d { new KeyGenDialog{*m_passBook, ui->passTable->currentIndex().row()} };
    d->show();
}

void PassBookForm::on_actionInsertAbove_triggered()
{
    m_passBook->insertRow(ui->passTable->currentIndex().row());
}

void PassBookForm::on_actionInsertBelow_triggered()
{
    m_passBook->insertRow(ui->passTable->currentIndex().row()+1);
}
