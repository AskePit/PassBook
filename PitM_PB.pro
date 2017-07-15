QT += core gui widgets

TARGET = PitM_PB
TEMPLATE = app

SOURCES += main.cpp\
    passbook.cpp \
    passbookform.cpp \
    hash.cpp \
    crypt.cpp \
    dialogs/keygendialog.cpp \
    utils.cpp \
    securetypes.cpp \
    dialogs/accountcreatedialog.cpp \
    dialogs/accountdeletedialog.cpp \
    dialogs/logindialog.cpp \
    dialogs/settingsdialog.cpp \
    settings.cpp

HEADERS  += \
    passbook.h \
    passbookform.h \
    platform.h \
    hash.h \
    crypt.h \
    dialogs/keygendialog.h \
    utils.h \
    securetypes.h \
    dialogs/accountcreatedialog.h \
    dialogs/accountdeletedialog.h \
    dialogs/logindialog.h \
    dialogs/settingsdialog.h \
    settings.h

FORMS    += \
    passbookform.ui \
    dialogs/keygendialog.ui \
    dialogs/accountcreatedialog.ui \
    dialogs/accountdeletedialog.ui \
    dialogs/logindialog.ui \
    dialogs/settingsdialog.ui

RESOURCES += \
    resources/i18n/lang.qrc

TRANSLATIONS += \
    resources/i18n/en.ts \
    resources/i18n/ru.ts

DEFINES *= QT_USE_QSTRINGBUILDER
