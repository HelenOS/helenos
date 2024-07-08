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

static void encode16_be(uint8_t *, uint16_t);
static void encode16_le(uint8_t *, uint16_t);
static void encode32_be(uint8_t *, uint32_t);
static void encode32_le(uint8_t *, uint32_t);
static uint16_t decode16_be(uint8_t *);
static uint16_t decode16_le(uint8_t *);
static uint32_t decode32_be(uint8_t *);
static uint32_t decode32_le(uint8_t *);

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

	rc = rndgen_uint32(rndgen, &uuid->time_low);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = rndgen_uint16(rndgen, &uuid->time_mid);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = rndgen_uint16(rndgen, &uuid->time_hi_and_version);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = rndgen_uint8(rndgen, &uuid->clock_seq_hi_and_reserved);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = rndgen_uint8(rndgen, &uuid->clock_seq_low);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	for (i = 0; i < _UUID_NODE_LEN; i++) {
		rc = rndgen_uint8(rndgen, &uuid->node[i]);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}
	}

	/* Version 4 UUID from random or pseudo-random numbers */
	uuid->time_hi_and_version = (uuid->time_hi_and_version & 0x0fff) | 0x4000;
	uuid->clock_seq_hi_and_reserved = (uuid->clock_seq_hi_and_reserved & 0x3f) | 0x80;

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

	encode32_be(buf, uuid->time_low);
	encode16_be(buf + 4, uuid->time_mid);
	encode16_be(buf + 6, uuid->time_hi_and_version);
	buf[8] = uuid->clock_seq_hi_and_reserved;
	buf[9] = uuid->clock_seq_low;

	for (i = 0; i < _UUID_NODE_LEN; i++)
		buf[10 + i] = uuid->node[i];
}

/** Decode UUID from binary form per RFC 4122.
 *
 * @param buf 16-byte source buffer
 * @param uuid Place to store UUID
 */
void uuid_decode(uint8_t *buf, uuid_t *uuid)
{
	int i;

	uuid->time_low = decode32_be(buf);
	uuid->time_mid = decode16_be(buf + 4);
	uuid->time_hi_and_version = decode16_be(buf + 6);
	uuid->clock_seq_hi_and_reserved = buf[8];
	uuid->clock_seq_low = buf[9];

	for (i = 0; i < _UUID_NODE_LEN; i++)
		uuid->node[i] = buf[10 + i];
}

/** Encode UUID into little-endian (GPT) binary form.
 *
 * @param uuid UUID
 * @param buf 16-byte destination buffer
 */
void uuid_encode_le(uuid_t *uuid, uint8_t *buf)
{
	int i;

	encode32_le(buf, uuid->time_low);
	encode16_le(buf + 4, uuid->time_mid);
	encode16_le(buf + 6, uuid->time_hi_and_version);
	buf[8] = uuid->clock_seq_hi_and_reserved;
	buf[9] = uuid->clock_seq_low;

	for (i = 0; i < _UUID_NODE_LEN; i++)
		buf[10 + i] = uuid->node[i];
}

/** Decode UUID from little-endian (GPT) binary form.
 *
 * @param buf 16-byte source buffer
 * @param uuid Place to store UUID
 */
void uuid_decode_le(uint8_t *buf, uuid_t *uuid)
{
	int i;

	uuid->time_low = decode32_le(buf);
	uuid->time_mid = decode16_le(buf + 4);
	uuid->time_hi_and_version = decode16_le(buf + 6);
	uuid->clock_seq_hi_and_reserved = buf[8];
	uuid->clock_seq_low = buf[9];

	for (i = 0; i < _UUID_NODE_LEN; i++)
		uuid->node[i] = buf[10 + i];
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

	uuid->time_low = time_low;

	uuid->time_mid = time_mid;

	uuid->time_hi_and_version = time_ver;

	uuid->clock_seq_hi_and_reserved = clock >> 8;

	uuid->clock_seq_low = clock & 0xff;

	for (i = 0; i < _UUID_NODE_LEN; i++)
		uuid->node[i] = (node >> 8 * (5 - i)) & 0xff;

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

	const char *format = "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x";
	if (uppercase)
		format = "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X";

	int ret = snprintf(str, size, format, uuid->time_low, uuid->time_mid,
	    uuid->time_hi_and_version, uuid->clock_seq_hi_and_reserved,
	    uuid->clock_seq_low, uuid->node[0], uuid->node[1], uuid->node[2],
	    uuid->node[3], uuid->node[4], uuid->node[5]);

	if (ret != 36) {
		free(str);
		return EINVAL;
	}

	*rstr = str;
	return EOK;
}

static void encode16_be(uint8_t *buf, uint16_t value)
{
	buf[0] = (value >> 8) & 0xff;
	buf[1] = value & 0xff;
}

static void encode16_le(uint8_t *buf, uint16_t value)
{
	buf[0] = value & 0xff;
	buf[1] = (value >> 8) & 0xff;
}

static void encode32_be(uint8_t *buf, uint32_t value)
{
	buf[0] = (value >> 24) & 0xff;
	buf[1] = (value >> 16) & 0xff;
	buf[2] = (value >> 8) & 0xff;
	buf[3] = value & 0xff;
}

static void encode32_le(uint8_t *buf, uint32_t value)
{
	buf[0] = value & 0xff;
	buf[1] = (value >> 8) & 0xff;
	buf[2] = (value >> 16) & 0xff;
	buf[3] = (value >> 24) & 0xff;
}

static uint16_t decode16_be(uint8_t *buf)
{
	return ((buf[0] << 8) | buf[1]);
}

static uint16_t decode16_le(uint8_t *buf)
{
	return ((buf[1] << 8) | buf[0]);
}

static uint32_t decode32_be(uint8_t *buf)
{
	return ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
}

static uint32_t decode32_le(uint8_t *buf)
{
	return ((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
}

/** @}
 */
