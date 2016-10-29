#include "masterkeeper.h"
#include "instruments.h"

MasterKeeper::MasterKeeper(byte* data, size_t size)
    : size(size)
    , data(data)
{
    x = new byte[size];
}

MasterKeeper::~MasterKeeper()
{
    delete x;
}

void MasterKeeper::lock()
{
    memrandomset(x, size);
    transform();
}

void MasterKeeper::unlock()
{
    transform();
}

void MasterKeeper::transform()
{
    for(uint i = 0; i<size; ++i) {
        data[i] ^= x[i];
    }
}
