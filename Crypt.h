/*
 * Crypt.h
 *
 *  Created on: 27.07.2015
 *      Author: nikolay shalakin
 *
 *  GOST 28147-89 gamming-crypt algorithm implementation
 *
 */

#ifndef GOST_CRYPT_INCLUDE
#define GOST_CRYPT_INCLUDE

#include "platform.h"

namespace gost
{
    constexpr size_t SIZE_OF_KEY = 32;

	class Crypter
	{
	public:
		Crypter();
		~Crypter();

        void cryptData(u8 *dst, const u8 *scr, size_t size, const u8 *password);
        void cryptString(u8 *dst, const char *scr, const u8 *password);
        void decryptString(char *dst, const u8 *scr, size_t size, const u8 *password);

		void useDefaultTable();
		void setTable(const char *filename); // file with 128 bytes representing SBox table for GOST encryption
        void setTable(const u8 *table);    // 128 bytes representing SBox table for GOST encryption

		void useDefaultSync();
		void setSync(const u64 sync);

	private:
		u32 SBox[4][256]; // this is an internal [4][256] representation of a standart [8][16] GOST table
		u32 Sync[2];
		u32 X[8]; // splitted key

		void cryptBlock(u32 &A, u32 &B);
		u32 f(u32 word);
	};
}

#endif //GOST_CRYPT_INCLUDE
