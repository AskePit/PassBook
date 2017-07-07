#ifndef PASSBOOKFORM_H
#define PASSBOOKFORM_H

#include "platform.h"
#include <QWidget>
#include <QMap>
#include <QModelIndex>
#include "securetypes.h"
#include "utils.h"

class PassBook;

namespace Ui {
class PassBookForm;
}

class PassBookForm : public QWidget
{
    Q_OBJECT

public:

    enum_class(Column) {
        Id = 0,
        Name,
        Url,
        Login,
        Password,
        End
    } enum_end;

    explicit PassBookForm(PassBook* passBook, QString login, const Master &master, QWidget *parent = 0);
    ~PassBookForm();

    void closeEvent(QCloseEvent *event);
    void print_notes();
    void sortIds();

private slots:
    void edit_password(QString &p);
    void gen_password(int n, PasswordType::type type);
    void on_addButton_clicked();
    void on_deleteButton_clicked();
    void on_upButton_clicked();
    void on_downButton_clicked();
    void on_saveButton_clicked();
    void on_backButton_clicked();
    void on_keyGen_clicked();
    void doubleClickReact(const QModelIndex& idx);

    void on_keyEdit_clicked();

private:
    Ui::PassBookForm *ui;

    QString login;
    Master master;
    PassBook* passBook;
    QMap<quint32, Password> picMap;

    void renderPasswordPixmap(QString &&p, int row);
};

#endif // PASSBOOKFORM_H
