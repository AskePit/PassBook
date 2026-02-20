#include "passbookform.h"
#include "ui_passbookform.h"

#include <QClipboard>
#include <QMouseEvent>
#include <QMenu>
#include <QLineEdit>
#include <QTableView>
#include <QPainter>
#include "logic/utils.h"
#include "logic/passbook.h"

#include "models/groupsModel.h"
#include "models/passwordsModel.h"

#include "forms/dialogs/logindialog.h"
#include "forms/dialogs/keygendialog.h"

bool TableEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    QTableView *table = qobject_cast<QTableView *>( qobject_cast<QWidget *>(watched)->parent() );

    if (table) {
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

    const bool show = index == m_hoveredPassword;

    const QRect &rect { option.rect };

    if(option.state & QStyle::State_Selected) {
        painter->fillRect(rect, QColor(0xF5, 0xF5, 0xF5));
    }

    QString pass { password.get() };

    if(pass.isEmpty()) {
        return;
    }

    if(show) {
        QFont font {QStringLiteral("Consolas"), 9};
        QFontMetrics fm {font};
        const int margin {4};
        int w { static_cast<int>(fm.averageCharWidth() * pass.size() + margin) };

        QPixmap pixmap {w, rect.height()};
        pixmap.fill();

        QPainter p(&pixmap);
        p.setFont(font);

        if(option.state & QStyle::State_Selected) {
            p.fillRect(0, 0, rect.width(), rect.height(), QColor(0xF5, 0xF5, 0xF5));
        }
        p.drawText(margin, margin, w, rect.height(), 0, pass);
        painter->drawPixmap(rect.x(), rect.y(), pixmap);
    } else {
        QPixmap pixmap {rect.width(), rect.height()};
        pixmap.fill();

        QPainter p {&pixmap};
        p.fillRect(0, 0, rect.width(), rect.height(), Qt::Dense4Pattern);
        painter->drawPixmap(option.rect.x(), rect.y(), pixmap);
    }
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
    , m_groupsModel(new GroupsModel(*passBook, this))
    , m_passwordsModel(new PasswordsModel(*passBook, this))
    , m_passwordsFilterModel(new PasswordsFilterModel(m_passwordsModel, this))
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

    ui->groupList->setModel(m_groupsModel);
    ui->passTable->setModel(m_passwordsFilterModel);

    connect(ui->groupList->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &curr, const QModelIndex &prev) {
        Q_UNUSED(prev);
        if(curr.isValid()) {
            m_passwordsModel->setGroup(curr.row());
            if(!ui->passTable->isEnabled()) {
                ui->passTable->setEnabled(true);
                for(int c = 0; c<Column::Count; ++c) {
                    ui->passTable->showColumn(c);
                }
            }
        } else {
            for(int c = 0; c<Column::Count; ++c) {
                ui->passTable->hideColumn(c);
            }
            ui->passTable->setDisabled(true);
        }

        checkListSelection(curr);
        checkTableSelection(curr);
    });
    ui->groupList->setCurrentIndex(m_groupsModel->index(0, 0));

    connect(ui->passTable->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &curr, const QModelIndex &prev) {
        Q_UNUSED(prev);
        checkTableSelection(curr);
    });

    ui->passTable->setItemDelegateForColumn(Column::Password, m_passBookDelegate);
    ui->passTable->setColumnWidth(Column::Name, 125);
    ui->passTable->setColumnWidth(Column::Url, 150);
    ui->passTable->setColumnWidth(Column::Login, 100);

    connect(ui->passTable, &QTableView::doubleClicked, this, &PassBookForm::doubleClickReact);
    connect(ui->passTable, &QTableView::clicked, this, &PassBookForm::checkTableSelection);
    connect(ui->groupList, &QListView::customContextMenuRequested, this, &PassBookForm::callGroupsContextMenu);
    connect(ui->passTable, &QTableView::customContextMenuRequested, this, &PassBookForm::callTableContextMenu);

    ui->filterLineEdit->setFocus();

    connect(filter, &TableEventFilter::tableHover, this, [this](QMouseEvent *event) {
        int c { ui->passTable->columnAt(event->position().x()) };
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
    });

    connect(filter, &TableEventFilter::tableClick, this, [=](QMouseEvent *event) {
        if(m_passBookDelegate->isInEditMode()) {
            return;
        }

        QModelIndex i { ui->passTable->indexAt(event->pos()) };
        ui->deletePassButton->setEnabled(i.isValid());
        checkTableSelection(i);
    });

    ui->groupList->viewport()->setAcceptDrops(true);
    ui->passTable->viewport()->setAcceptDrops(true);
    restoreGeometry(iniSettings.value(QStringLiteral("MainFormGeometry")).toByteArray());

    restoreCurrentGroup();
    deselectPass();
}

PassBookForm::~PassBookForm()
{
    QClipboard *clipboard { QApplication::clipboard() };
    clipboard->clear();
    delete m_passBook;
    delete m_groupsModel;
    delete m_passwordsModel;
    delete m_passBookDelegate;
    delete ui;
}

void PassBookForm::on_addPassButton_clicked()
{
    m_passwordsModel->insertRow(m_passwordsModel->rowCount());
}

void PassBookForm::on_deletePassButton_clicked()
{
    QModelIndex index { ui->passTable->currentIndex() };
    m_passwordsModel->removeRow(index.row(), index.parent());
}

void PassBookForm::on_addGroupButton_clicked()
{
    int i = m_groupsModel->rowCount();
    m_groupsModel->insertRow(i);
    ui->groupList->clearSelection();
    ui->groupList->selectionModel()->setCurrentIndex(m_groupsModel->index(i, 0), QItemSelectionModel::Select);
}

void PassBookForm::on_deleteGroupButton_clicked()
{
    QModelIndex index { ui->groupList->currentIndex() };
    m_groupsModel->removeRow(index.row(), index.parent());
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
    QModelIndex index = m_passwordsFilterModel->mapToSource(idx);
    if(index.column() == Column::Password) {
        m_passBookDelegate->informDoubleClicked();
        QClipboard *clipboard { QApplication::clipboard() };

        size_t group = m_passwordsModel->getGroup();
        SecureString &&pass { m_passBook->getPassword(group, index.row()) };
        clipboard->setText( QString{std::move(pass)} );
    }
}

void PassBookForm::checkTableSelection(const QModelIndex& idx)
{
    if(!idx.isValid()) {
        deselectPass();
    }
}

void PassBookForm::checkListSelection(const QModelIndex& idx)
{
    Q_UNUSED(idx);
    ui->deleteGroupButton->setEnabled(idx.isValid());
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
    menu.addAction(ui->actionInsertPassAbove);
    menu.addAction(ui->actionInsertPassBelow);

    menu.exec(globalPos);
}

void PassBookForm::callGroupsContextMenu(const QPoint &point)
{
    QModelIndex index { ui->groupList->indexAt(point) };
    if(!index.isValid()) {
        return;
    }

    QPoint globalPos { ui->groupList->mapToGlobal(point) };
    QMenu menu(ui->groupList);
    menu.addAction(ui->actionInsertGroupAbove);
    menu.addAction(ui->actionInsertGroupBelow);

    menu.exec(globalPos);
}

void PassBookForm::deselectPass()
{
    ui->passTable->clearSelection();
    const QModelIndex index;
    ui->passTable->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select);
    ui->deletePassButton->setDisabled(true);
}

void PassBookForm::saveCurrentGroup()
{
    QVariant group = m_groupsModel->data(ui->groupList->currentIndex());
    iniSettings.setValue("CurrentGroup", group);
}

void PassBookForm::restoreCurrentGroup()
{
    QString group = iniSettings.value("CurrentGroup").toString();
    if(group.isNull()) {
        return;
    }

    QModelIndex i = m_groupsModel->groupIndex(group);
    ui->groupList->setCurrentIndex(i);
}

void PassBookForm::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    QClipboard *clipboard { QApplication::clipboard() };
    clipboard->clear();

    iniSettings.setValue(QStringLiteral("MainFormGeometry"), saveGeometry());
    saveCurrentGroup();

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
    QModelIndex noteIndex { m_passwordsFilterModel->mapToSource(ui->passTable->currentIndex()) };
    KeyGenDialog *d { new KeyGenDialog{*m_passBook, m_passwordsModel->getGroup(), static_cast<size_t>(noteIndex.row())} };
    d->show();
}

void PassBookForm::on_actionInsertPassAbove_triggered()
{
    QModelIndex index { ui->passTable->currentIndex() };
    m_passwordsModel->insertRow(index.row(), index.parent());
}

void PassBookForm::on_actionInsertPassBelow_triggered()
{
    QModelIndex index { ui->passTable->currentIndex() };
    m_passwordsModel->insertRow(index.row()+1, index.parent());
}

void PassBookForm::on_actionInsertGroupAbove_triggered()
{
    QModelIndex index { ui->groupList->currentIndex() };
    m_groupsModel->insertRow(index.row(), index.parent());
}

void PassBookForm::on_actionInsertGroupBelow_triggered()
{
    QModelIndex index { ui->groupList->currentIndex() };
    m_groupsModel->insertRow(index.row()+1, index.parent());
}

void PassBookForm::on_filterLineEdit_textEdited(const QString &filterString)
{
    const bool stayInGroup = ui->filterInGroupCheckBox->isChecked();

    if (filterString.isEmpty() || stayInGroup) {
        m_passwordsModel->setGroup(ui->groupList->currentIndex().row());
        ui->addPassButton->setEnabled(true);
        ui->deletePassButton->setEnabled(true);
    } else {
        m_passwordsModel->setGroup(ALL_GROUPS);
        ui->addPassButton->setEnabled(false);
        ui->deletePassButton->setEnabled(false);
    }

    m_passwordsFilterModel->setFilterString(filterString);
}

void PassBookForm::on_filterInGroupCheckBox_toggled(bool checked)
{
    Q_UNUSED(checked);
    on_filterLineEdit_textEdited(ui->filterLineEdit->text());
}


void PassBookForm::on_filterResetButton_clicked()
{
    ui->filterLineEdit->clear();
    on_filterLineEdit_textEdited(ui->filterLineEdit->text());
}

