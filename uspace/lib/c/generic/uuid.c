/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/** @file Universaly Unique Identifier (see RFC 4122)
 */

#include <errno.h>
#include <uuid.h>
#include <stdlib.h>

/** Generate UUID.
 *
 * @param uuid Place to store generated UUID
 * @return EOK on success or negative error code
 */
int uuid_generate(uuid_t *uuid)
{
	int i;
	struct timeval tv;

	/* XXX This is a rather poor way of generating random numbers */
	gettimeofday(&tv, NULL);
	srandom(tv.tv_sec ^ tv.tv_usec);

	for (i = 0; i < uuid_bytes; i++)
		uuid->b[i] = random();

	/* Version 4 UUID from random or pseudo-random numbers */
	uuid->b[8] = (uuid->b[8] & ~0xc0) | 0x40;
	uuid->b[6] = (uuid->b[6] & 0xf0) | 0x40;

	return EOK;
}

/** Encode UUID into binary form per RFC 4122.
 *
 * @param uuid UUID
 * @param buf 16-byte destination buffer
 */
void uuid_encode(uuid_t *uuid, uint8_t *buf)
{
	int i;

	for (i = 0; i < uuid_bytes; i++)
		buf[i] = uuid->b[i];
}

/** Decode UUID from binary form per RFC 4122.
 *
 * @param buf 16-byte source buffer
 * @param uuid Place to store UUID
 */
void uuid_decode(uint8_t *buf, uuid_t *uuid)
{
	int i;

	for (i = 0; i < uuid_bytes; i++)
		uuid->b[i] = buf[i];
}

/** @}
 */
