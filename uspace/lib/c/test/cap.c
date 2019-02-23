/*
 * Copyright (c) 2019 Matthieu Riolo
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

#include <pcut/pcut.h>
#include <cap.h>

PCUT_INIT;

PCUT_TEST_SUITE(cap);

PCUT_TEST(cap_format)
{
	int block_size = 4;
	size_t block[] = {
		0,
		1,
		2,
		10,
	};

	int input_size = 5;
	size_t input[] = {
		0,
		10,
		100,
		1000,
		1000000,
		1000000000,
	};

	const char *out[] = {
		"0 B",
		"0 B",
		"0 B",
		"0 B",

		"0 B",
		"10 B",
		"20 B",
		"100 B",

		"0 B",
		"100 B",
		"200 B",
		"1.000 kB",

		"0 B",
		"1.000 kB",
		"2.000 kB",
		"10.00 kB",

		"0 B",
		"1.000 MB",
		"2.000 MB",
		"10.00 MB",

		"0 B",
		"1.000 GB",
		"2.000 GB",
		"10.00 GB",
	};

	cap_spec_t cap;
	char *str;
	errno_t rc;

	int x, i;
	for (i = 0; i < input_size; i++) {
		for (x = 0; x < block_size; x++) {
			cap_from_blocks(input[i], block[x], &cap);
			cap_simplify(&cap);

			rc = cap_format(&cap, &str);

			PCUT_ASSERT_ERRNO_VAL(EOK, rc);
			PCUT_ASSERT_STR_EQUALS(out[x + (block_size * i)], str);
			free(str);

			cap_from_blocks(block[x], input[i], &cap);
			cap_simplify(&cap);

			rc = cap_format(&cap, &str);

			PCUT_ASSERT_ERRNO_VAL(EOK, rc);
			PCUT_ASSERT_STR_EQUALS(out[x + (block_size * i)], str);
			free(str);
		}
	}
}

PCUT_TEST(cap_format_rounding)
{
	int input_size = 8;
	uint64_t input[] = {
		555,
		5555,
		55555,
		555555555,
		5555555555,
		555999999,
		5999999,
		999999
	};

	const char *out[] = {
		"555 B",
		"5.555 kB",
		"55.56 kB",
		"555.6 MB",
		"5.556 GB",
		"556.0 MB",
		"6.000 MB",
		"1.000 MB",
	};

	cap_spec_t cap;
	char *str;
	errno_t rc;

	int i;
	for (i = 0; i < input_size; i++) {
		cap_from_blocks(input[i], 1, &cap);
		cap_simplify(&cap);

		rc = cap_format(&cap, &str);

		PCUT_ASSERT_ERRNO_VAL(EOK, rc);
		PCUT_ASSERT_STR_EQUALS(out[i], str);
		free(str);

		cap_from_blocks(1, input[i], &cap);
		cap_simplify(&cap);

		rc = cap_format(&cap, &str);

		PCUT_ASSERT_ERRNO_VAL(EOK, rc);
		PCUT_ASSERT_STR_EQUALS(out[i], str);
		free(str);
	}
}

PCUT_TEST(cap_parse)
{
	int input_size = 4;
	const char *input[] = {
		"0 B",
		"100 B",
		"1 kB",
		"1.555 kB",
	};

	int out_cunit[] = {
		cu_byte,
		cu_byte,
		cu_kbyte,
		cu_kbyte,
	};

	int out_dp[] = {
		0,
		0,
		0,
		3,
	};

	int out_m[] = {
		0,
		100,
		1,
		1555,
	};

	cap_spec_t cap;
	errno_t rc;
	int i;

	for (i = 0; i < input_size; i++) {
		rc = cap_parse(input[i], &cap);

		PCUT_ASSERT_ERRNO_VAL(EOK, rc);
		PCUT_ASSERT_INT_EQUALS(out_cunit[i], cap.cunit);
		PCUT_ASSERT_INT_EQUALS(out_dp[i], cap.dp);
		PCUT_ASSERT_INT_EQUALS(out_m[i], cap.m);
	}
}

PCUT_TEST(cap_to_blocks)
{
	int input_size = 0;
	int input_m[] = {
		0,
		1,
		1000,
		5555,
		7777,
	};

	int input_dp[] = {
		0,
		0,
		3,
		3,
		2,
	};

	int block[] = {
		1,
		1,
		1,
		2,
		3,
	};

	int out_nom[] = {
		0,
		1000,
		1000,
		2778,
		25923,
	};

	int out_min[] = {
		0,
		1000,
		1000,
		2777,
		25923,
	};

	int out_max[] = {
		0,
		1000,
		1000,
		2778,
		25924,
	};

	cap_spec_t cap;
	errno_t rc;
	int i;
	uint64_t ret;

	for (i = 0; i < input_size; i++) {
		cap.m = input_m[i];
		cap.dp = input_dp[i];
		cap.cunit = cu_kbyte;

		rc = cap_to_blocks(&cap, cv_nom, block[i], &ret);
		PCUT_ASSERT_ERRNO_VAL(EOK, rc);
		PCUT_ASSERT_INT_EQUALS(out_nom[i], ret);

		rc = cap_to_blocks(&cap, cv_min, block[i], &ret);
		PCUT_ASSERT_ERRNO_VAL(EOK, rc);
		PCUT_ASSERT_INT_EQUALS(out_min[i], ret);

		rc = cap_to_blocks(&cap, cv_max, block[i], &ret);
		PCUT_ASSERT_ERRNO_VAL(EOK, rc);
		PCUT_ASSERT_INT_EQUALS(out_max[i], ret);
	}
}

PCUT_EXPORT(cap);
