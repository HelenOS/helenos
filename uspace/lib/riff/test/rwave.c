/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mem.h>
#include <pcut/pcut.h>
#include <riff/rwave.h>
#include <stdio.h>
#include <stdint.h>

PCUT_INIT;

PCUT_TEST_SUITE(rwave);

/** Write a WAVE file and read it back */
PCUT_TEST(write_read)
{
	char fname[L_tmpnam];
	char *p;
	uint16_t samples[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	uint16_t rsamples[10];
	size_t nread;
	rwavew_t *ww = NULL;
	rwaver_t *wr = NULL;
	rwave_params_t params;
	rwave_params_t rparams;
	errno_t rc;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	/* Write RIFF wAVE file */

	params.channels = 2;
	params.bits_smp = 16;
	params.smp_freq = 44100;

	rc = rwave_wopen(p, &params, &ww);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ww);

	rc = rwave_write_samples(ww, (void *) samples, sizeof(samples));
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = rwave_wclose(ww);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Read back RIFF WAVE file */

	rc = rwave_ropen(p, &rparams, &wr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wr);

	PCUT_ASSERT_INT_EQUALS(params.channels, rparams.channels);
	PCUT_ASSERT_INT_EQUALS(params.bits_smp, rparams.bits_smp);
	PCUT_ASSERT_INT_EQUALS(params.smp_freq, rparams.smp_freq);

	rc = rwave_read_samples(wr, rsamples, sizeof(rsamples), &nread);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(sizeof(samples), nread);

	PCUT_ASSERT_INT_EQUALS(0, memcmp(samples, rsamples, sizeof(samples)));

	rc = rwave_rclose(wr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	(void) remove(p);
}

PCUT_EXPORT(rwave);
