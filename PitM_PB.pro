QT += core gui widgets multimedia

TARGET = PitM_PB
TEMPLATE = app

SOURCES += main.cpp\
    passbook.cpp \
    passbookform.cpp \
    hash.cpp \
    crypt.cpp \
    dialogs/profilecreatedialog.cpp \
    dialogs/profiledeletedialog.cpp \
    dialogs/passworddialog.cpp \
    dialogs/keygendialog.cpp \
    dialogs/keyeditdialog.cpp \
    utils.cpp \
    securetypes.cpp

HEADERS  += \
    passbook.h \
    passbookform.h \
    platform.h \
    hash.h \
    crypt.h \
    dialogs/profilecreatedialog.h \
    dialogs/profiledeletedialog.h \
    dialogs/passworddialog.h \
    dialogs/keygendialog.h \
    dialogs/keyeditdialog.h \
    utils.h \
    securetypes.h

FORMS    += \
    passbookform.ui \
    dialogs/profilecreatedialog.ui \
    dialogs/profiledeletedialog.ui \
    dialogs/passworddialog.ui \
    dialogs/keygendialog.ui \
    dialogs/keyeditdialog.ui
