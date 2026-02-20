#pragma once

#include "logic/settings.h"

#include <QMainWindow>
#include <QSettings>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QMainWindow
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);
    ~LoginDialog();

private slots:
    void loadAccounts();

    void onCreateButtonClicked();
    void onDeleteButtonClicked();
    void onEnterButtonClicked();
    void onActionAboutTriggered();
    void onActionSettingsTriggered();

    void deleteAccount();

protected:
    void closeEvent(QCloseEvent *e);

private:
    Ui::LoginDialog *ui;

    void createAccount(const QString &log, QString &&key);
    QString currentAccountFile(QString newLogin = QString());
};
