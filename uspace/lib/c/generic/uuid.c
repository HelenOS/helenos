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
#include <rndgen.h>
#include <stdlib.h>
#include <stddef.h>
#include <str.h>
#include <stdio.h>

/** Generate UUID.
 *
 * @param uuid Place to store generated UUID
 * @return EOK on success or an error code
 */
errno_t uuid_generate(uuid_t *uuid)
{
	int i;
	rndgen_t *rndgen;
	errno_t rc;

	rc = rndgen_create(&rndgen);
	if (rc != EOK)
		return EIO;

	for (i = 0; i < uuid_bytes; i++) {
		rc = rndgen_uint8(rndgen, &uuid->b[i]);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}
	}

	/* Version 4 UUID from random or pseudo-random numbers */
	uuid->b[6] = (uuid->b[6] & 0x0f) | 0x40;
	uuid->b[8] = (uuid->b[8] & 0x3f) | 0x80;

error:
	rndgen_destroy(rndgen);
	return rc;
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

/** Parse string UUID.
 *
 * If @a endptr is not NULL, it is set to point to the first character
 * following the UUID. If @a endptr is NULL, the string must not contain
 * any characters following the UUID, otherwise an error is returned.
 *
 * @param str    String beginning with UUID string representation
 * @param uuid   Place to store UUID
 * @param endptr Place to store pointer to end of UUID or @c NULL
 *
 * @return EOK on success or an error code
 */
errno_t uuid_parse(const char *str, uuid_t *uuid, const char **endptr)
{
	errno_t rc;
	const char *eptr;
	uint32_t time_low;
	uint16_t time_mid;
	uint16_t time_ver;
	uint16_t clock;
	uint64_t node;
	int i;

	rc = str_uint32_t(str, &eptr, 16, false, &time_low);
	if (rc != EOK || eptr != str + 8 || *eptr != '-')
		return EINVAL;

	rc = str_uint16_t(str + 9, &eptr, 16, false, &time_mid);
	if (rc != EOK || eptr != str + 13 || *eptr != '-')
		return EINVAL;

	rc = str_uint16_t(str + 14, &eptr, 16, false, &time_ver);
	if (rc != EOK || eptr != str + 18 || *eptr != '-')
		return EINVAL;

	rc = str_uint16_t(str + 19, &eptr, 16, false, &clock);
	if (rc != EOK || eptr != str + 23 || *eptr != '-')
		return EINVAL;

	rc = str_uint64_t(str + 24, &eptr, 16, false, &node);
	if (rc != EOK || eptr != str + 36)
		return EINVAL;

	uuid->b[0] = time_low >> 24;
	uuid->b[1] = (time_low >> 16) & 0xff;
	uuid->b[2] = (time_low >> 8) & 0xff;
	uuid->b[3] = time_low & 0xff;

	uuid->b[4] = time_mid >> 8;
	uuid->b[5] = time_mid & 0xff;

	uuid->b[6] = time_ver >> 8;
	uuid->b[7] = time_ver & 0xff;

	uuid->b[8] = clock >> 8;
	uuid->b[9] = clock & 0xff;

	for (i = 0; i < 6; i++)
		uuid->b[10 + i] = (node >> 8 * (5 - i)) & 0xff;

	if (endptr != NULL) {
		*endptr = str + 36;
	} else {
		if (*(str + 36) != '\0')
			return EINVAL;
	}

	return EOK;
}

/** Format UUID into string representation.
 *
 * @param uuid UUID
 * @param rstr Place to store pointer to newly allocated string
 *
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t uuid_format(uuid_t *uuid, char **rstr, bool uppercase)
{
	size_t size = 37;
	char *str = malloc(sizeof(char) * size);
	if (str == NULL)
		return ENOMEM;

	const char *format = "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x";
	if (uppercase)
		format = "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X";

	int ret = snprintf(str, size, format, uuid->b[0], uuid->b[1], uuid->b[2], uuid->b[3], uuid->b[4], uuid->b[5], uuid->b[6], uuid->b[7], uuid->b[8], uuid->b[9], uuid->b[10], uuid->b[11], uuid->b[12], uuid->b[13], uuid->b[14], uuid->b[15]);

	if (ret != 36)
		return EINVAL;

	*rstr = str;
	return EOK;
}

/** @}
 */
