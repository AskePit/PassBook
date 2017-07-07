#ifndef PASSBOOK_H
#define PASSBOOK_H

#include "securetypes.h"
#include <QVector>

struct Note
{
    QString source;
    QString URL;
    QString login;
    Password password;
};

class PassBook {
public:
    PassBook(const QString &fileName);
    int verify(const Master &master); // returns sizeOfFile OR -1 in case of failure
    bool load(const Master &master);
    void save(const Master &master);
    QVector<Note>& getNotes() { return m_notes; }

private:
    bool m_loaded;
    QVector<Note> m_notes;
    QString m_fileName;
};

#endif //PASSBOOK_H
