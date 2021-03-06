#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

signals:
    void languageChanged();
    void accountsPathChanged();

private slots:
    void on_buttonBox_accepted();
    void setDefaultPath();

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
