#ifndef MASTERKEEPER_H
#define MASTERKEEPER_H

#include "platform.h"

class MasterKeeper
{
public:
    MasterKeeper(byte* data, uint size);
    ~MasterKeeper();

    void lock();
    void unlock();

private:
    uint size;
    byte* data;
    byte* x;

    void transform();
};

#endif // MASTERKEEPER_H
