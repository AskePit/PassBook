QT += core gui widgets multimedia

TARGET = PitM_PB
TEMPLATE = app


SOURCES += main.cpp\
    instruments.cpp \
    passbook.cpp \
    passbookform.cpp \
    passworddialog.cpp \
    keygendialog.cpp \
    profiledeletedialog.cpp \
    profilecreatedialog.cpp \
    hash.cpp \
    crypt.cpp \
    passwordkeeper.cpp \
    masterkeeper.cpp \
    keyeditdialog.cpp

HEADERS  += \
    instruments.h \
    passbook.h \
    passbookform.h \
    passworddialog.h \
    keygendialog.h \
    profiledeletedialog.h \
    profilecreatedialog.h \
    platform.h \
    hash.h \
    crypt.h \
    passwordkeeper.h \
    masterkeeper.h \
    keyeditdialog.h

FORMS    += \
    passbookform.ui \
    passworddialog.ui \
    keygendialog.ui \
    profiledeletedialog.ui \
    profilecreatedialog.ui \
    keyeditdialog.ui
