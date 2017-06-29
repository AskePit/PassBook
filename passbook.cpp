#include "PassBook.h"
#include "Crypt.h"
#include "Hash.h"
#include <fstream>
#include <sstream>

//const size_t PassBook::SIZEOF_KEY = 32;

const char PassBook::SOURCE_END = 0x7;
const char PassBook::URL_END    = 0x8;
const char PassBook::LOGIN_END  = 0x9;
const char PassBook::PASS_END   = 0xA;

PassBook::PassBook(std::string fileName)
    : loaded(false)
    , fileName(fileName)
{}

int PassBook::verify(const byte* password)
{
    using namespace gost;

    std::ifstream in(fileName.c_str(), std::fstream::in | std::ifstream::ate | std::ifstream::binary);
    size_t sizeofFile = (size_t)in.tellg();
    in.seekg(0, in.beg);

    if(sizeofFile < SIZE_OF_HASH) {
        in.close();
        return false;
    }

    byte fileHash[SIZE_OF_HASH];
    in.read((char*)fileHash, SIZE_OF_HASH);
    in.close();

    byte realHash[SIZE_OF_HASH];
    hash(realHash, password, PassBook::SIZE_OF_KEY);

    return memcmp(fileHash, realHash, SIZE_OF_HASH) == 0 ?
           static_cast<int>(sizeofFile - SIZE_OF_HASH) :
           -1;
}

bool PassBook::load(const byte* password)
{
    using namespace gost;

    int sizeofMessage = verify(password);
    if(sizeofMessage < 0) {
        return loaded = false;
    }

    std::fstream f(fileName.c_str(), std::fstream::in | std::fstream::binary);
    std::streambuf* rawBuff = f.rdbuf();
    rawBuff->pubseekpos(SIZE_OF_HASH, f.in);

    byte* cryptedMessage = new byte[sizeofMessage];
    byte* decryptedMessage = new byte[sizeofMessage];

    rawBuff->sgetn((char*)cryptedMessage, sizeofMessage);
    f.close();

    Crypter crypter;
    crypter.decryptString((char*)decryptedMessage, cryptedMessage, sizeofMessage, password);

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
        case PASS_END:   note.password.load((char*)cutHead, password);
                             notes.push_back(note);
                             break;
            default: break;
        }

        cutHead = cutCursor;
    }

    delete [] cryptedMessage;

    return loaded = true;
}

void PassBook::save(const byte* password)
{
    using namespace gost;
    std::stringstream ss;

    for(uint i = 0; i<notes.size(); ++i) {
        ss << notes[i].source.toStdString();
        ss.write(&SOURCE_END, 1);
        ss << notes[i].URL.toStdString();
        ss.write(&URL_END, 1);
        ss << notes[i].login.toStdString();
        ss.write(&LOGIN_END, 1);
        ss << notes[i].password.getPass(password).toStdString();
        ss.write(&PASS_END, 1);
    }

    std::string s = ss.str();
    const size_t size = s.length();

    const char* decryptedMessage = s.c_str();
    byte* cryptedMessage = new byte[size];


    Crypter crypter;
    crypter.cryptString(cryptedMessage, decryptedMessage, password);

    std::fstream f(fileName.c_str(), std::fstream::out | std::fstream::trunc | std::fstream::binary);

    byte fileHash[SIZE_OF_HASH];
    hash(fileHash, password, PassBook::SIZE_OF_KEY);

    f.write((char*)fileHash, SIZE_OF_HASH);
    f.write((char*)cryptedMessage, size);
    f.close();

    delete [] cryptedMessage;
}
