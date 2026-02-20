#pragma once

#include "logic/stdExt.h"

#include <QVersionNumber>
#include <QSettings>
#include <QTranslator>

enum_class(Language) {
    English,
    Russian

enum_interface
    static const QList<Language::type> &enumerate();
    static QString toString(Language::type t);
    static Language::type fromString(const QString &str);
    static QString i18nName(Language::type t);
    static Language::type fromi18nName(const QString &str);
};

struct Settings
{
    const QVersionNumber version;
    Language::type language;
    QString accountsPath;

    QTranslator *translator;
    QTranslator *qtTranslator;

    Settings();
};

void loadSettings();
void storeSettings();

extern Settings appSettings;
extern QSettings iniSettings;
