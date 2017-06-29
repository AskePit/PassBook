QT += core gui widgets multimedia

TARGET = PitM_PB
TEMPLATE = app

SOURCES += main.cpp\
    instruments.cpp \
    passbook.cpp \
    passbookform.cpp \
    hash.cpp \
    crypt.cpp \
    passwordkeeper.cpp \
    masterkeeper.cpp \
    dialogs/profilecreatedialog.cpp \
    dialogs/profiledeletedialog.cpp \
    dialogs/passworddialog.cpp \
    dialogs/keygendialog.cpp \
    dialogs/keyeditdialog.cpp

HEADERS  += \
    instruments.h \
    passbook.h \
    passbookform.h \
    platform.h \
    hash.h \
    crypt.h \
    passwordkeeper.h \
    masterkeeper.h \
    dialogs/profilecreatedialog.h \
    dialogs/profiledeletedialog.h \
    dialogs/passworddialog.h \
    dialogs/keygendialog.h \
    dialogs/keyeditdialog.h

FORMS    += \
    passbookform.ui \
    dialogs/profilecreatedialog.ui \
    dialogs/profiledeletedialog.ui \
    dialogs/passworddialog.ui \
    dialogs/keygendialog.ui \
    dialogs/keyeditdialog.ui
