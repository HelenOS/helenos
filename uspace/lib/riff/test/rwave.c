/*
 * Copyright (c) 2020 Jiri Svoboda
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
