#include "passbook.h"
#include "crypt.h"
#include "hash.h"
#include "utils.h"
#include <QFileInfo>

PassBook::PassBook(const QString &fileName)
    : m_loaded(false)
    , m_fileName(fileName)
{}

static inline quint64 fileSize(const QString &fileName)
{
    QFileInfo info(fileName);
    return info.size();
}

int PassBook::verify(const Master &master)
{
    using namespace gost;

    quint64 sizeofFile = fileSize(m_fileName);

    if(sizeofFile < SIZE_OF_HASH) {
        return false;
    }

    SecureBytes fileHash(SIZE_OF_HASH);

    QFile in(m_fileName);
    in.open(QIODevice::ReadOnly);
    in.read(as<char *>(fileHash), SIZE_OF_HASH);
    in.close();

    SecureBytes realHash(SIZE_OF_HASH);
    MasterDoor door(master);
    hash(as<byte*>(realHash), door.get(), gost::SIZE_OF_KEY);

    return fileHash == realHash ? static_cast<int>(sizeofFile - SIZE_OF_HASH)
                                : -1;
}

static const char SOURCE_END = 0x7;
static const char URL_END    = 0x8;
static const char LOGIN_END  = 0x9;
static const char PASS_END   = 0xA;

static void parseData(const SecureBytes &data, QVector<Note> &notes, const Master &master)
{
    const char* head = data.data();
    const char* cursor = head;
    const char* end = head + data.size();
    Note note;

    while(cursor != end) {
        if(*cursor < SOURCE_END || *cursor > PASS_END) {
            ++cursor;
            continue;
        }

        byte code = *cursor;

        SecureBytes fieldData(data.mid(head - data.data(), cursor - head));
        QString str(std::move(fieldData));

        switch(code) {
            case SOURCE_END: note.source = str; break;
            case URL_END:    note.URL    = str; break;
            case LOGIN_END:  note.login  = str; break;
            case PASS_END:   note.password.load(std::move(str), master);
                             notes.push_back(note);
                             break;
            default: break;
        }

        head = ++cursor;
    }
}

bool PassBook::load(const Master &master)
{
    using namespace gost;

    int sizeofMessage = verify(master);
    if(sizeofMessage < 0) {
        return m_loaded = false;
    }

    QFile f(m_fileName);
    f.open(QIODevice::ReadOnly);
    f.seek(SIZE_OF_HASH);

    SecureBytes cryptedData(sizeofMessage);
    SecureBytes data(sizeofMessage);

    f.read(as<char*>(cryptedData), sizeofMessage);
    f.close();

    {
        Crypter crypter;
        MasterDoor door(master);
        crypter.cryptData(as<byte*>(data), as<byte*>(cryptedData), sizeofMessage, door.get());
    }

    parseData(data, m_notes, master);

    return m_loaded = true;
}

void PassBook::save(const Master &master)
{
    using namespace gost;

    SecureBytes data;

    for(const auto &note : m_notes) {
        data += note.source;
        data += SOURCE_END;
        data += note.URL;
        data += URL_END;
        data += note.login;
        data += LOGIN_END;
        data += note.password.get(master);
        data += PASS_END;
    }

    const int size = data.size();

    SecureBytes cryptedData(size);
    SecureBytes fileHash(SIZE_OF_HASH);

    {
        Crypter crypter;
        MasterDoor door(master);
        crypter.cryptData(as<byte*>(cryptedData), as<byte*>(data), size, door.get());
        hash(as<byte*>(fileHash), door.get(), gost::SIZE_OF_KEY);
    }

    QFile f(m_fileName);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(as<char*>(fileHash), SIZE_OF_HASH);
    f.write(as<char*>(cryptedData), size);
    f.close();
}
