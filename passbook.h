#ifndef PASSBOOK_H
#define PASSBOOK_H

#include "platform.h"

#include <QString>
#include <vector>

#include "passwordkeeper.h"

struct Note
{
    QString source;
    QString URL;
    QString login;
    PasswordKeeper password;
};

class PassBook {
public:
    static const size_t SIZE_OF_KEY = 32;

    static const char SOURCE_END;
    static const char URL_END;
    static const char LOGIN_END;
    static const char PASS_END;

    PassBook(std::string fileName);
    int verify(const byte* password); // returns sizeOfFile OR -1 in case of failure
    bool load(const byte* password);
    void save(const byte* password);
    std::vector<Note>& getNotes() { return notes; }

private:
    bool loaded;
    std::vector<Note> notes;
    std::string fileName;
};

#endif //PASSBOOK_H
