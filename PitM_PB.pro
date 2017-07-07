QT += core gui widgets multimedia

TARGET = PitM_PB
TEMPLATE = app

SOURCES += main.cpp\
    passbook.cpp \
    passbookform.cpp \
    hash.cpp \
    crypt.cpp \
    dialogs/passworddialog.cpp \
    dialogs/keygendialog.cpp \
    dialogs/keyeditdialog.cpp \
    utils.cpp \
    securetypes.cpp \
    dialogs/accountcreatedialog.cpp \
    dialogs/accountdeletedialog.cpp

HEADERS  += \
    passbook.h \
    passbookform.h \
    platform.h \
    hash.h \
    crypt.h \
    dialogs/passworddialog.h \
    dialogs/keygendialog.h \
    dialogs/keyeditdialog.h \
    utils.h \
    securetypes.h \
    dialogs/accountcreatedialog.h \
    dialogs/accountdeletedialog.h

FORMS    += \
    passbookform.ui \
    dialogs/passworddialog.ui \
    dialogs/keygendialog.ui \
    dialogs/keyeditdialog.ui \
    dialogs/accountcreatedialog.ui \
    dialogs/accountdeletedialog.ui

DEFINES *= QT_USE_QSTRINGBUILDER
