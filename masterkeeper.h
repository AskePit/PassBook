#ifndef MASTERKEEPER_H
#define MASTERKEEPER_H

#include "platform.h"

class MasterKeeper
{
public:
    MasterKeeper(byte* data, size_t size);
    ~MasterKeeper();

    void lock();
    void unlock();

private:
    size_t m_size;
    byte* m_data;
    byte* m_x;

    void transform();
};

#endif // MASTERKEEPER_H
