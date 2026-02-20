#include <cstddef>
#include "platform.h"

namespace gost
{
    const size_t SIZE_OF_HASH = 64;
    const size_t SIZE_OF_SALT = 6;
    void hash(u8* hash, const u8* message, const uint length);
}
