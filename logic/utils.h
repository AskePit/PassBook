#pragma once

#include <QApplication>
#include <QMessageBox>
#include "stdExt.h"

class QWidget;
class SecureBytes;

void memrandomset(u8* data, size_t size);
void memrandomset(SecureBytes &bytes);

enum_class (PasswordType) {
    Letters = 0,
    LettersAndDigits,
    Standard,
    Advanced

enum_interface
    static QString toString(PasswordType::type t) {
        switch(t) {
            case Letters:          return QApplication::tr("Letters");
            case LettersAndDigits: return QApplication::tr("Letters & Digits");
            default:
            case Standard:         return QApplication::tr("Standard");
            case Advanced:         return QApplication::tr("Advanced");
        }
    }

    static const QList<PasswordType::type> &enumerate() {
        static const QList<PasswordType::type> l {
            Letters, LettersAndDigits, Standard, Advanced
        };
        return l;
    }
};

int callQuestionDialog(const QString &message, QWidget *parent = nullptr);
void callInfoDialog(const QString &message, QWidget *parent = nullptr);
bool copyFileForced(const QString &from, const QString &to);

QString passGenerate(int n, PasswordType::type type);

#define wipememory2(_ptr, _set, _len) do {              \
        volatile char *_vptr = (volatile char *)(_ptr);	\
        size_t _vlen = (_len);                          \
        unsigned char _vset = (_set);                   \
        while(_vlen) {                                  \
            *_vptr = (_vset);                           \
            _vptr++;                                    \
            _vlen--;                                    \
        }                                               \
    } while(0)

#define wipememory(_ptr,_len) wipememory2(_ptr,0,_len)
