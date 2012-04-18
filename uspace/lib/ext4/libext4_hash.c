/*
 * Copyright (c) 2012 Frantisek Princ
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libext4
 * @{
 */ 

/**
 * @file	libext4_hash.c
 * @brief	Hashing algorithms for ext4 HTree.
 */

#include <errno.h>
#include <mem.h>

#include "libext4.h"

#define TEA_DELTA 0x9E3779B9


/* F, G and H are basic MD4 functions: selection, majority, parity */
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) (((x) & (y)) + (((x) ^ (y)) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))


/*
 * The generic round function.  The application is so specific that
 * we don't bother protecting all the arguments with parens, as is generally
 * good macro practice, in favor of extra legibility.
 * Rotation is separate from addition to prevent recomputation
 */
#define ROUND(f, a, b, c, d, x, s)      \
        (a += f(b, c, d) + x, a = (a << s) | (a >> (32 - s)))
#define K1 0
#define K2 013240474631UL
#define K3 015666365641UL


static void tea_transform(uint32_t buf[4], uint32_t const in[])
{
	uint32_t sum = 0;
	uint32_t b0 = buf[0], b1 = buf[1];
	uint32_t a = in[0], b = in[1], c = in[2], d = in[3];
	int n = 16;

	do {
		sum += TEA_DELTA;
		b0 += ((b1 << 4)+a) ^ (b1+sum) ^ ((b1 >> 5)+b);
		b1 += ((b0 << 4)+c) ^ (b0+sum) ^ ((b0 >> 5)+d);
	} while (--n);

	buf[0] += b0;
	buf[1] += b1;
}


static void half_md4_transform(uint32_t buf[4], const uint32_t in[8])
{
	uint32_t a = buf[0], b = buf[1], c = buf[2], d = buf[3];

	/* Round 1 */
	ROUND(F, a, b, c, d, in[0] + K1,  3);
	ROUND(F, d, a, b, c, in[1] + K1,  7);
	ROUND(F, c, d, a, b, in[2] + K1, 11);
	ROUND(F, b, c, d, a, in[3] + K1, 19);
	ROUND(F, a, b, c, d, in[4] + K1,  3);
	ROUND(F, d, a, b, c, in[5] + K1,  7);
	ROUND(F, c, d, a, b, in[6] + K1, 11);
	ROUND(F, b, c, d, a, in[7] + K1, 19);

	/* Round 2 */
	ROUND(G, a, b, c, d, in[1] + K2,  3);
	ROUND(G, d, a, b, c, in[3] + K2,  5);
	ROUND(G, c, d, a, b, in[5] + K2,  9);
	ROUND(G, b, c, d, a, in[7] + K2, 13);
	ROUND(G, a, b, c, d, in[0] + K2,  3);
	ROUND(G, d, a, b, c, in[2] + K2,  5);
	ROUND(G, c, d, a, b, in[4] + K2,  9);
	ROUND(G, b, c, d, a, in[6] + K2, 13);

	/* Round 3 */
	ROUND(H, a, b, c, d, in[3] + K3,  3);
	ROUND(H, d, a, b, c, in[7] + K3,  9);
	ROUND(H, c, d, a, b, in[2] + K3, 11);
	ROUND(H, b, c, d, a, in[6] + K3, 15);
	ROUND(H, a, b, c, d, in[1] + K3,  3);
	ROUND(H, d, a, b, c, in[5] + K3,  9);
	ROUND(H, c, d, a, b, in[0] + K3, 11);
	ROUND(H, b, c, d, a, in[4] + K3, 15);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;

}

static uint32_t hash_unsigned(const char *name, int len)
{
	uint32_t hash, hash0 = 0x12a3fe2d, hash1 = 0x37abe8f9;
	const unsigned char *ucp = (const unsigned char *) name;

	while (len--) {
		hash = hash1 + (hash0 ^ (((int) *ucp++) * 7152373));

		if (hash & 0x80000000) {
			hash -= 0x7fffffff;
		}
		hash1 = hash0;
		hash0 = hash;
	}
	return hash0 << 1;
}

static uint32_t hash_signed(const char *name, int len)
{
	uint32_t hash, hash0 = 0x12a3fe2d, hash1 = 0x37abe8f9;
	const signed char *scp = (const signed char *) name;

	while (len--) {
		hash = hash1 + (hash0 ^ (((int) *scp++) * 7152373));

		if (hash & 0x80000000) {
			hash -= 0x7fffffff;
		}
		hash1 = hash0;
		hash0 = hash;
	}
	return hash0 << 1;
}

static void str2hashbuf_signed(const char *msg, int len, uint32_t *buf, int num)
{
	uint32_t pad, val;
	int i;
	const signed char *scp = (const signed char *) msg;

	pad = (uint32_t)len | ((uint32_t)len << 8);
	pad |= pad << 16;

	val = pad;
	if (len > num*4) {
		len = num * 4;
	}

	for (i = 0; i < len; i++) {
		if ((i % 4) == 0) {
			val = pad;
		}
		val = ((int) scp[i]) + (val << 8);
		if ((i % 4) == 3) {
			*buf++ = val;
			val = pad;
			num--;
		}
	}

	if (--num >= 0) {
		*buf++ = val;
	}

	while (--num >= 0) {
		*buf++ = pad;
	}
}


static void str2hashbuf_unsigned(const char *msg, int len, uint32_t *buf, int num)
{
	uint32_t pad, val;
	int i;
	const unsigned char *ucp = (const unsigned char *) msg;

	pad = (uint32_t)len | ((uint32_t)len << 8);
	pad |= pad << 16;

	val = pad;
	if (len > num*4) {
        len = num * 4;
	}

	for (i = 0; i < len; i++) {
		if ((i % 4) == 0) {
			val = pad;
		}
		val = ((int) ucp[i]) + (val << 8);
		if ((i % 4) == 3) {
			*buf++ = val;
			val = pad;
			num--;
		}
	}

	if (--num >= 0) {
		*buf++ = val;
	}

	while (--num >= 0) {
		*buf++ = pad;
	}
}

int ext4_hash_string(ext4_hash_info_t *hinfo, int len, const char *name)
{
	uint32_t hash = 0;
	uint32_t minor_hash = 0;
	const char *p;
    int i;
    uint32_t in[8], buf[4];
    void (*str2hashbuf)(const char *, int, uint32_t *, int) = str2hashbuf_signed;

    /* Initialize the default seed for the hash checksum functions */
	buf[0] = 0x67452301;
	buf[1] = 0xefcdab89;
	buf[2] = 0x98badcfe;
	buf[3] = 0x10325476;

    /* Check if the seed is all zero's */
	if (hinfo->seed) {
		for (i = 0; i < 4; i++) {
			if (hinfo->seed[i] != 0) {
            	break;
			}
		}
		if (i < 4) {
			memcpy(buf, hinfo->seed, sizeof(buf));
		}
    }

	switch (hinfo->hash_version) {
		case EXT4_HASH_VERSION_LEGACY_UNSIGNED:
			hash = hash_unsigned(name, len);
			break;

		case EXT4_HASH_VERSION_LEGACY:
			hash = hash_signed(name, len);
			break;


		case EXT4_HASH_VERSION_HALF_MD4_UNSIGNED:
			str2hashbuf = str2hashbuf_unsigned;

		case EXT4_HASH_VERSION_HALF_MD4:
			p = name;
			while (len > 0) {
				(*str2hashbuf)(p, len, in, 8);
				half_md4_transform(buf, in);
				len -= 32;
				p += 32;
			}
			minor_hash = buf[2];
			hash = buf[1];
			break;


		case EXT4_HASH_VERSION_TEA_UNSIGNED:
			str2hashbuf = str2hashbuf_unsigned;

		case EXT4_HASH_VERSION_TEA:
			p = name;
			while (len > 0) {
				(*str2hashbuf)(p, len, in, 4);
				tea_transform(buf, in);
				len -= 16;
				p += 16;
			}
			hash = buf[0];
			minor_hash = buf[1];
			break;

		default:
			hinfo->hash = 0;
			return EINVAL;
	}

	hash = hash & ~1;
	if (hash == (EXT4_DIRECTORY_HTREE_EOF << 1)) {
		hash = (EXT4_DIRECTORY_HTREE_EOF-1) << 1;
	}

	hinfo->hash = hash;
	hinfo->minor_hash = minor_hash;

	return EOK;
}

/**
 * @}
 */ 
