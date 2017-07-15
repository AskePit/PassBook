#include "settings.h"

#include <QTranslator>
#include <QLibraryInfo>

QList<Language::type> Language::enumerate() {
    return {English, Russian};
}

QString Language::toString(Language::type t) {
    switch(t) {
        default:
        case English: return QApplication::tr("English");
        case Russian: return QApplication::tr("Russian");
    }
}

Language::type Language::fromString(const QString &str)
{
    if(str == QApplication::tr("Russian")) {
        return Russian;
    } else {
        return English;
    }
}

QString Language::i18nName(Language::type t) {
    switch(t) {
        default:
        case English: return "en";
        case Russian: return "ru";
    }
}

Language::type Language::fromi18nName(const QString &str)
{
    if(str == "ru") {
        return Russian;
    } else {
        return English;
    }
}

Settings::Settings()
    : version(1, 0, 0)
    , language(Language::English)
    , accountsPath("")
    , translator(nullptr)
    , qtTranslator(nullptr)
{}

Settings appSettings;
QSettings iniSettings {"settings.ini", QSettings::IniFormat};

static void changeLanguage()
{
    auto &&translator = appSettings.translator;
    auto &&qtTranslator = appSettings.qtTranslator;
    QString lang = Language::i18nName(appSettings.language);

    if(translator) {
        qApp->removeTranslator(translator);
        delete translator;
    }

    translator = new QTranslator();
    if (translator->load(lang, ":/i18n")) {
        qApp->installTranslator(translator);
    }

    if(qtTranslator) {
        qApp->removeTranslator(qtTranslator);
        delete qtTranslator;
    }
    qtTranslator = new QTranslator();
    if (qtTranslator->load("qtbase_" + lang, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        qApp->installTranslator(qtTranslator);
    }
}

void loadSettings()
{
    appSettings.language = Language::fromi18nName( iniSettings.value("Language", "en").toString() );
    appSettings.accountsPath = iniSettings.value("AccountsPath", "").toString();
    changeLanguage();
}

void storeSettings()
{
    iniSettings.setValue("Language", Language::i18nName(appSettings.language));
    iniSettings.setValue("AccountsPath", appSettings.accountsPath);
    changeLanguage();
}
