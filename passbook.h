#ifndef PASSBOOK_H
#define PASSBOOK_H

#include "platform.h"

#include <QString>
#include <QVector>

#include "securetypes.h"

struct Note
{
    QString source;
    QString URL;
    QString login;
    Password password;
};

class PassBook {
public:
    static const char SOURCE_END;
    static const char URL_END;
    static const char LOGIN_END;
    static const char PASS_END;

    PassBook(const QString &fileName);
    int verify(Master &master); // returns sizeOfFile OR -1 in case of failure
    bool load(Master &master);
    void save(Master &master);
    QVector<Note>& getNotes() { return m_notes; }

private:
    bool m_loaded;
    QVector<Note> m_notes;
    QString m_fileName;
};

#endif //PASSBOOK_H
