/*
 * Copyright (c) 2018 Jiri Svoboda
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
/**
 * @file
 * @brief Random number generator.
 *
 * Generate random (as opposed to pseudorandom) numbers. This should be
 * used sparingly (e.g. to seed a pseudorandom number generator).
 */

#include <errno.h>
#include <rndgen.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/** Create random number generator.
 *
 * @param rrndgen Place to store new random number generator
 * @return EOK on success or error code
 */
errno_t rndgen_create(rndgen_t **rrndgen)
{
	rndgen_t *rndgen;
	struct timespec ts;

	rndgen = calloc(1, sizeof(rndgen_t));
	if (rndgen == NULL)
		return ENOMEM;

	/* XXX This is a rather poor way of generating random numbers */
	getuptime(&ts);
	rndgen->seed = ts.tv_sec ^ ts.tv_nsec;

	*rrndgen = rndgen;
	return EOK;
}

/** Destroy random number generator.
 *
 * @param rndgen Random number generator or @c NULL
 */
void rndgen_destroy(rndgen_t *rndgen)
{
	if (rndgen == NULL)
		return;

	free(rndgen);
}

/** Generate random 8-bit integer.
 *
 * @param rndgen Random number generator
 * @param rb Place to store random 8-bit integer
 * @return EOK on success or error code
 */
errno_t rndgen_uint8(rndgen_t *rndgen, uint8_t *rb)
{
	rndgen->seed = (1366 * rndgen->seed + 150889) % 714025;

	*rb = rndgen->seed & 0xff;
	return EOK;
}

/** Generate random 16-bit integer.
 *
 * @param rndgen Random number generator
 * @param rw Place to store random 16-bit integer
 * @return EOK on success or error code
 */
errno_t rndgen_uint16(rndgen_t *rndgen, uint16_t *rw)
{
	int i;
	uint8_t b;
	uint16_t w;
	errno_t rc;

	w = 0;
	for (i = 0; i < 2; i++) {
		rc = rndgen_uint8(rndgen, &b);
		if (rc != EOK)
			return rc;

		w = (w << 8) | b;
	}

	*rw = w;
	return EOK;
}

/** Generate random 32-bit integer.
 *
 * @param rndgen Random number generator
 * @param rw Place to store random 32-bit integer
 * @return EOK on success or error code
 */
errno_t rndgen_uint32(rndgen_t *rndgen, uint32_t *rw)
{
	int i;
	uint8_t b;
	uint32_t w;
	errno_t rc;

	w = 0;
	for (i = 0; i < 4; i++) {
		rc = rndgen_uint8(rndgen, &b);
		if (rc != EOK)
			return rc;

		w = (w << 8) | b;
	}

	*rw = w;
	return EOK;
}

/** @}
 */
