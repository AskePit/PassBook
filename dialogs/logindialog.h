#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include "settings.h"

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

    void on_createButton_clicked();
    void on_deleteButton_clicked();
    void on_enterButton_clicked();
    void on_actionAbout_triggered();
    void on_actionSettings_triggered();

    void createAccount(const QString &log, QString &key);
    void deleteAccount();

protected:
    void closeEvent(QCloseEvent *e);

private:
    Ui::LoginDialog *ui;

    QString currentAccountFile(QString newLogin = QString());
};

#endif // PASSWORDDIALOG_H
