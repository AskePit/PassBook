#ifndef INSTRUMENTS_H
#define INSTRUMENTS_H

#include <QApplication>
#include <QMessageBox>
#include "platform.h"

class QWidget;
class SecureBytes;

void memrandomset(u8* data, size_t size);
void memrandomset(SecureBytes &bytes);

template<class OUT, class IN>
inline auto as(IN *data) {
    return reinterpret_cast<
        typename std::conditional<std::is_const<IN>::value, const OUT, OUT>::type
    >(data);
}

template <class OUT, class IN>
inline auto as(IN &t) -> decltype(as<OUT>(t.data())) {
    return as<OUT>(t.data());
}

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

/*
 * Macro wipememory, wipememory2, fast_wipememory2 implementations is
 * part of Libgcrypt.
 *
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2005 2007, 2011
 *               Free Software Foundation, Inc.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser general Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 */

/*
 * To avoid that a compiler optimizes certain memset calls away, these
 * macros may be used instead.
 */
#define wipememory2(_ptr, _set, _len) do {				\
        volatile char *_vptr = (volatile char *)(_ptr);		\
        size_t _vlen = (_len);					\
        unsigned char _vset = (_set);				\
        fast_wipememory2(_vptr, _vset, _vlen);			\
        while(_vlen) {						\
            *_vptr = (_vset);				\
            _vptr++;					\
            _vlen--;					\
        }							\
    } while(0)

#define wipememory(_ptr,_len) wipememory2(_ptr,0,_len)

/*
 * Optimized fast_wipememory2 for i386 and x86-64 architechtures.  Maybe leave
 * tail bytes unhandled, in which case tail bytes are handled by wipememory2.
 */
#if defined(__x86_64__) && __GNUC__ >= 4
#define fast_wipememory2(_vptr,_vset,_vlen) do {		\
        unsigned long long int _vset8 = _vset;		\
        if (_vlen < 8)					\
            break;					\
        _vset8 *= 0x0101010101010101ULL;		\
        do {						\
            asm volatile("movq %[set], %[ptr]\n\t"	\
                     : /**/			\
                     : [set] "Cr" (_vset8),	\
                       [ptr] "m" (*_vptr)	\
                     : "memory");		\
            _vlen -= 8;				\
            _vptr += 8;				\
        } while (_vlen >= 8);				\
    } while (0)
#elif defined (__i386__) && SIZEOF_UNSIGNED_LONG == 4 && __GNUC__ >= 4
#define fast_wipememory2(_ptr,_set,_len) do {			\
        unsigned long _vset4 = _vset;			\
        if (_vlen < 4)					\
            break;					\
        _vset4 *= 0x01010101;				\
        do {						\
            asm volatile("movl %[set], %[ptr]\n\t"	\
                     : /**/			\
                     : [set] "Cr" (_vset4),	\
                       [ptr] "m" (*_vptr)	\
                     : "memory");		\
            _vlen -= 4;				\
            _vptr += 4;				\
        } while (_vlen >= 4);				\
    } while (0)
#else
#define fast_wipememory2(_ptr,_set,_len)
#endif

#endif //INSTRUMENTS_H
