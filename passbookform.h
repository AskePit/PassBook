#ifndef PASSBOOKFORM_H
#define PASSBOOKFORM_H

#include "platform.h"
#include <QWidget>
#include <QMap>
#include <QModelIndex>
#include "securetypes.h"

class PassBook;

namespace Ui {
class PassBookForm;
}

class PassBookForm : public QWidget
{
    Q_OBJECT

public:

    enum ColumnName {
        NUMBER_COL,
        RESOURCE_COL,
        URL_COL,
        LOGIN_COL,
        PASSWORD_COL,
        END_OF_COL
    };

    explicit PassBookForm(PassBook* passBook, QString login, const Master &master, QWidget *parent = 0);
    ~PassBookForm();

    void closeEvent(QCloseEvent *event);
    void print_notes();
    void sortIDs();

private slots:
    void edit_password(QString &p);
    void gen_password(int n, int mode);
    void on_addButton_clicked();
    void on_deleteButton_clicked();
    void on_UP_clicked();
    void on_DOWN_clicked();
    void on_SAVE_clicked();
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
