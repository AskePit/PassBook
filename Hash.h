/*
* stribog_data.h
*
*  Created on: Feb 15, 2013
*      Author: Oleksandr Kazymyrov
*		Acknowledgments: Oleksii Shevchuk
*/

#ifndef STRIBOG_H_
#define STRIBOG_H_

#include "platform.h"

namespace GOST
{
    const uint SIZEOF_HASH = 64;
    void hash(byte* hash, const byte* message, const uint length);
}

#endif /* STRIBOG_H_ */
