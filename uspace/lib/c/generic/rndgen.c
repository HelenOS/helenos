/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
