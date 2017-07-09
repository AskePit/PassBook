#ifndef PASSBOOKFORM_H
#define PASSBOOKFORM_H

#include <QWidget>

class PassBook;

namespace Ui {
class PassBookForm;
}

class PassBookForm : public QWidget
{
    Q_OBJECT

public:
    explicit PassBookForm(PassBook* passBook, QString login, QWidget *parent = 0);
    ~PassBookForm();

    void closeEvent(QCloseEvent *event);

private slots:
    void on_addButton_clicked();
    void on_deleteButton_clicked();
    void on_upButton_clicked();
    void on_downButton_clicked();
    void on_backButton_clicked();
    void on_keyGen_clicked();
    void on_keyEdit_clicked();
    void on_actionSave_triggered();

    void save();
    void doubleClickReact(const QModelIndex& idx);
    void enableControls(int row);

private:
    Ui::PassBookForm *ui;

    QString login;
    PassBook* passBook;
};

#endif // PASSBOOKFORM_H
