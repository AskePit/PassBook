#include "PassBook.h"
#include "Crypt.h"
#include "Hash.h"
#include <QFileInfo>
#include <QStringBuilder>

//const size_t PassBook::SIZEOF_KEY = 32;

const char PassBook::SOURCE_END = 0x7;
const char PassBook::URL_END    = 0x8;
const char PassBook::LOGIN_END  = 0x9;
const char PassBook::PASS_END   = 0xA;

PassBook::PassBook(const QString &fileName)
    : m_loaded(false)
    , m_fileName(fileName)
{}

static inline quint64 fileSize(const QString &fileName)
{
    QFileInfo info(fileName);
    return info.size();
}

int PassBook::verify(Master &master)
{
    using namespace gost;

    quint64 sizeofFile = fileSize(m_fileName);

    if(sizeofFile < SIZE_OF_HASH) {
        return false;
    }

    byte fileHash[SIZE_OF_HASH];

    QFile in(m_fileName);
    in.open(QIODevice::ReadOnly);
    in.read(reinterpret_cast<char *>(fileHash), SIZE_OF_HASH);
    in.close();

    byte realHash[SIZE_OF_HASH];
    {
        MasterDoor door(master);
        hash(realHash, door.get(), gost::SIZE_OF_KEY);
    }

    return memcmp(fileHash, realHash, SIZE_OF_HASH) == 0
            ? static_cast<int>(sizeofFile - SIZE_OF_HASH)
            : -1;
}

bool PassBook::load(Master &master)
{
    using namespace gost;

    int sizeofMessage = verify(master);
    if(sizeofMessage < 0) {
        return m_loaded = false;
    }

    QFile f(m_fileName);
    f.open(QIODevice::ReadOnly);
    f.seek(SIZE_OF_HASH);

    byte* cryptedMessage = new byte[sizeofMessage];
    byte* decryptedMessage = new byte[sizeofMessage];

    f.read(reinterpret_cast<char*>(cryptedMessage), sizeofMessage);
    f.close();

    {
        Crypter crypter;
        MasterDoor door(master);
        crypter.decryptString((char*)decryptedMessage, cryptedMessage, sizeofMessage, door.get());
    }

    byte* cutHead   = decryptedMessage;
    byte* cutCursor = decryptedMessage;
    byte* cursorEnd = decryptedMessage + sizeofMessage;
    Note note;

    while(cutCursor != cursorEnd) {
        if(*cutCursor < SOURCE_END || *cutCursor > PASS_END) {
            ++cutCursor;
            continue;
        }

        byte code = *cutCursor;

        *cutCursor = 0x0;
        ++cutCursor;

        switch(code) {
            case SOURCE_END: note.source = (char*)cutHead; break;
            case URL_END:    note.URL    = (char*)cutHead; break;
            case LOGIN_END:  note.login  = (char*)cutHead; break;
            case PASS_END:   note.password.load((char*)cutHead, master);
                             m_notes.push_back(note);
                             break;
            default: break;
        }

        cutHead = cutCursor;
    }

    delete [] cryptedMessage;

    return m_loaded = true;
}

void PassBook::save(Master &master)
{
    using namespace gost;

    QByteArray bytes;

    for(const auto &note : m_notes) {
        bytes += note.source;
        bytes += SOURCE_END;
        bytes += note.URL;
        bytes += URL_END;
        bytes += note.login;
        bytes += LOGIN_END;
        bytes += note.password.get(master);
        bytes += PASS_END;
    }

    const int size = bytes.size();

    const char* decryptedMessage = bytes.data();
    byte* cryptedMessage = new byte[size];
    byte fileHash[SIZE_OF_HASH];

    {
        Crypter crypter;
        MasterDoor door(master);
        crypter.cryptString(cryptedMessage, decryptedMessage, door.get());
        hash(fileHash, door.get(), gost::SIZE_OF_KEY);
    }

    QFile f(m_fileName);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write((char*)fileHash, SIZE_OF_HASH);
    f.write((char*)cryptedMessage, size);
    f.close();

    delete [] cryptedMessage;
}
