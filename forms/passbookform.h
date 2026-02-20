#pragma once

#include <QMainWindow>
#include <QSettings>

class PassBook;
class GroupsModel;
class PasswordsModel;
class PasswordsFilterModel;

namespace Ui {
class PassBookForm;
}

class PassBookDelegate;

class PassBookForm : public QMainWindow
{
    Q_OBJECT

public:
    explicit PassBookForm(PassBook* m_passBook, QWidget *parent = 0);
    ~PassBookForm();

    void closeEvent(QCloseEvent *event);

private slots:
    void onAddPassButtonClicked();
    void onDeletePassButtonClicked();
    void onAddGroupButtonClicked();
    void onDeleteGroupButtonClicked();
    void onBackButtonClicked();
    void onActionSaveTriggered();
    void onActionEditPasswordTriggered();
    void onActionGeneratePasswordTriggered();
    void onActionInsertPassAboveTriggered();
    void onActionInsertPassBelowTriggered();
    void onActionInsertGroupAboveTriggered();
    void onActionInsertGroupBelowTriggered();
    void onFilterLineEditTextEdited(const QString &filterString);
    void onFilterInGroupCheckBoxToggled(bool checked);
    void onFilterResetButtonClicked();

    void save();
    void doubleClickReact(const QModelIndex& idx);
    void checkTableSelection(const QModelIndex& idx);
    void checkListSelection(const QModelIndex& idx);
    void callTableContextMenu(const QPoint &point);
    void callGroupsContextMenu(const QPoint &point);
    void deselectPass();

    void saveCurrentGroup();
    void restoreCurrentGroup();

private:
    Ui::PassBookForm *ui;

    PassBook* m_passBook;
    GroupsModel* m_groupsModel;
    PasswordsModel* m_passwordsModel;
    PasswordsFilterModel* m_passwordsFilterModel;
    PassBookDelegate* m_passBookDelegate;

    bool m_closeWithBack;
};
