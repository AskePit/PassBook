#include "masterkeeper.h"
#include "instruments.h"

MasterKeeper::MasterKeeper(byte* data, size_t size)
    : m_size(size)
    , m_data(data)
{
    m_x = new byte[m_size];
}

MasterKeeper::~MasterKeeper()
{
    delete m_x;
}

void MasterKeeper::lock()
{
    memrandomset(m_x, m_size);
    transform();
}

void MasterKeeper::unlock()
{
    transform();
}

void MasterKeeper::transform()
{
    for(uint i = 0; i<m_size; ++i) {
        m_data[i] ^= m_x[i];
    }
}
