#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);
    ~LoginDialog();

private slots:
    void on_createButton_clicked();
    void on_deleteButton_clicked();
    void on_enterButton_clicked();

    void createAccount(const QString &log, QString &key);
    void deleteAccount();

protected:
    void closeEvent(QCloseEvent *e);

private:
    Ui::LoginDialog *ui;
    QSettings m_settings;
};

#endif // PASSWORDDIALOG_H
