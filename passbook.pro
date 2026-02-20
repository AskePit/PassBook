QT += core gui widgets

TARGET = passbook
TEMPLATE = app

CONFIG += c++20

SOURCES += main.cpp \
    forms/dialogs/accountcreatedialog.cpp \
    forms/dialogs/keygendialog.cpp \
    forms/dialogs/logindialog.cpp \
    forms/dialogs/settingsdialog.cpp \
    forms/passbookform.cpp \
    logic/crypt.cpp \
    logic/hash.cpp \
    logic/passbook.cpp \
    logic/securetypes.cpp \
    logic/settings.cpp \
    logic/utils.cpp \
    models/groupsModel.cpp \
    models/passwordsModel.cpp

HEADERS  += \
    forms/dialogs/accountcreatedialog.h \
    forms/dialogs/keygendialog.h \
    forms/dialogs/logindialog.h \
    forms/dialogs/settingsdialog.h \
    forms/passbookform.h \
    logic/crypt.h \
    logic/hash.h \
    logic/passbook.h \
    logic/securetypes.h \
    logic/settings.h \
    logic/stdExt.h \
    logic/utils.h \
    models/groupsModel.h \
    models/passwordsModel.h

FORMS    += \
    forms/dialogs/accountcreatedialog.ui \
    forms/dialogs/keygendialog.ui \
    forms/dialogs/logindialog.ui \
    forms/dialogs/settingsdialog.ui \
    forms/passbookform.ui

RESOURCES += \
    resources/i18n/lang.qrc \
    resources/resouces.qrc

TRANSLATIONS += \
    resources/i18n/en.ts \
    resources/i18n/ru.ts

DEFINES *= QT_USE_QSTRINGBUILDER

DISTFILES += \
    resources/logo_transparent.png \
    resources/logo_white.png

RC_ICONS = resources/logo_white.ico
