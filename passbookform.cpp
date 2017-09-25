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
            emit tableHover(mouseEvent);
        } else if(event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent { static_cast<QMouseEvent *>(event) };
            emit tableClick(mouseEvent);
        }
    }

    return QObject::eventFilter(watched, event);
}

PassBookDelegate::PassBookDelegate(QWidget *parent)
    : QStyledItemDelegate(parent)
    , m_hoveredPassword(QModelIndex())
    , m_inEditMode(false)
    , m_doubleClicked(false)
{}

void PassBookDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Password password { qvariant_cast<Password>(index.data()) };
    password.paint(painter, option, index == m_hoveredPassword);
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

    ui->groupList->viewport()->setMouseTracking(true);
    ui->passTable->viewport()->setMouseTracking(true);
    ui->passTable->viewport()->installEventFilter(filter);

    ui->groupList->setModel(passBook);
    ui->passTable->setModel(passBook);

    connect(ui->groupList, &QListView::clicked, [this](const QModelIndex &index){
        ui->passTable->setRootIndex(index);
        clickReact(index);
    });

    ui->groupList->setCurrentIndex(passBook->index(0, 0));
    emit ui->groupList->clicked(passBook->index(0, 0));

    ui->passTable->setItemDelegateForColumn(Column::Password, m_passBookDelegate);
    ui->passTable->setColumnWidth(Column::Name, 125);
    ui->passTable->setColumnWidth(Column::Url, 150);
    ui->passTable->setColumnWidth(Column::Login, 100);

    connect(ui->passTable, &QTableView::doubleClicked, this, &PassBookForm::doubleClickReact);
    connect(ui->passTable, &QTableView::clicked, this, &PassBookForm::clickReact);
    connect(ui->groupList, &QListView::customContextMenuRequested, this, &PassBookForm::callTableContextMenu);
    connect(ui->passTable, &QTableView::customContextMenuRequested, this, &PassBookForm::callTableContextMenu);

    ui->passTable->setFocus();

    connect(filter, &TableEventFilter::tableHover, [=](QMouseEvent *event) {
        int c { ui->passTable->columnAt(event->x()) };
        bool isPassword { c == Column::Password };

        if(m_passBookDelegate->isInEditMode()) {
            return;
        }

        if(!isPassword) {
            m_passBookDelegate->setHoveredPassword(QModelIndex());
        } else {
            QModelIndex i = ui->passTable->indexAt(event->pos());
            m_passBookDelegate->setHoveredPassword(i);
        }

        // trigger passwords to be repainted in case of comming from/to horizontal header
        emit passBook->dataChanged(passBook->index(0, Column::Password), passBook->index(passBook->rowCount()-1, Column::Password), {Qt::DisplayRole});
    });

    connect(filter, &TableEventFilter::tableClick, [=](QMouseEvent *event) {
        if(m_passBookDelegate->isInEditMode()) {
            return;
        }

        QModelIndex i { ui->passTable->indexAt(event->pos()) };
        ui->deleteButton->setEnabled(i.isValid());
        if(!i.isValid()) {
            deselectAll();
        }
    });

    ui->groupList->viewport()->setAcceptDrops(true);
    ui->passTable->viewport()->setAcceptDrops(true);
    restoreGeometry(iniSettings.value("MainFormGeometry").toByteArray());

    deselectAll();
}

PassBookForm::~PassBookForm()
{
    QClipboard *clipboard { QApplication::clipboard() };
    clipboard->clear();
    delete m_passBook;
    delete ui;
}

void PassBookForm::on_addButton_clicked()
{
    if(ui->groupList->hasFocus()) {
        m_passBook->insertRow(m_passBook->rowCount());
    } else {
        auto groupIndex = ui->groupList->currentIndex();
        m_passBook->insertRow(m_passBook->rowCount(groupIndex), groupIndex);
    }
}

void PassBookForm::on_deleteButton_clicked()
{
    QModelIndex index { ui->groupList->hasFocus() ? ui->groupList->currentIndex() : ui->passTable->currentIndex() };
    m_passBook->removeRow(index.row(), index.parent());
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
    if(idx.parent().isValid() && idx.column() == Column::Password) {
        m_passBookDelegate->informDoubleClicked();
        QClipboard *clipboard { QApplication::clipboard() };

        noteid id {idx.internalId()};
        SecureString &&pass { m_passBook->getPassword(id.groupIndex(), idx.row()) };
        clipboard->setText( QString{std::move(pass)} );
    }
}

void PassBookForm::clickReact(const QModelIndex& idx)
{
    if(!idx.isValid()) {
        ui->passTable->clearSelection();
    }
}

void PassBookForm::callTableContextMenu(const QPoint &point)
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

void PassBookForm::callGroupsContextMenu(const QPoint &pos)
{
    /*QModelIndex index { ui->passTable->indexAt(point) };
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

    menu.exec(globalPos);*/
}

void PassBookForm::deselectAll()
{
    ui->passTable->clearSelection();
    const QModelIndex index;
    ui->passTable->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select);
    ui->deleteButton->setDisabled(true);
}

void PassBookForm::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    QClipboard *clipboard { QApplication::clipboard() };
    clipboard->clear();

    iniSettings.setValue("MainFormGeometry", saveGeometry());

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
    QModelIndex index { ui->passTable->currentIndex() };
    KeyGenDialog *d { new KeyGenDialog{*m_passBook, index.parent().row(), index.row()} };
    d->show();
}

void PassBookForm::on_actionInsertAbove_triggered()
{
    QModelIndex index { ui->passTable->currentIndex() };
    m_passBook->insertRow(index.row(), index.parent());
}

void PassBookForm::on_actionInsertBelow_triggered()
{
    QModelIndex index { ui->passTable->currentIndex() };
    m_passBook->insertRow(index.row()+1, index.parent());
}
