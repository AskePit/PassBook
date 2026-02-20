#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "logindialog.h"

#include <QFileInfo>
#include <QFileDialog>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    connect(ui->customButton, &QRadioButton::toggled, [this](bool checked){
        ui->pathEdit->setEnabled(checked);
        ui->browseButton->setEnabled(checked);
        if(!checked) {
            setDefaultPath();
        }
    });

    for(auto t : Language::enumerate()) {
        ui->languageComboBox->addItem(Language::toString(t), t);
    }
    ui->languageComboBox->setCurrentText(Language::toString(appSettings.language));

    QFileInfo info {appSettings.accountsPath};
    ui->pathEdit->setText(info.absoluteFilePath());

    if(appSettings.accountsPath.isEmpty()) {
        ui->defaultButton->setChecked(true);
    } else {
        ui->customButton->setChecked(true);
    }

    connect(ui->browseButton, &QPushButton::clicked, [this]() {
        QString newPath { QFileDialog::getExistingDirectory(this, tr("Choose accounts folder"), ui->pathEdit->text()) };
        if(!newPath.isNull()) {
            ui->pathEdit->setText(newPath);
        }
    });
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setDefaultPath()
{
    QFileInfo info {QStringLiteral("")};
    ui->pathEdit->setText(info.absoluteFilePath());
}

void SettingsDialog::on_buttonBox_accepted()
{
    bool langChanged = false;
    bool pathChanged = false;

    Language::type lang = static_cast<Language::type>( ui->languageComboBox->currentData().toInt() );
    langChanged = lang != appSettings.language;
    if(langChanged) {
        appSettings.language = lang;
    }

    bool defaultPath = ui->defaultButton->isChecked();
    QString accountsPath = defaultPath ? QStringLiteral("") : ui->pathEdit->text();
    pathChanged = accountsPath != appSettings.accountsPath;
    if(pathChanged) {
        appSettings.accountsPath = accountsPath;
    }

    storeSettings();

    if(langChanged) {
        emit languageChanged();
    }
    if(pathChanged) {
        emit accountsPathChanged();
    }

    QDialog::accept();
}
