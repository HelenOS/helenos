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
/**
 * @file Storage capacity specification.
 */

#include <cap.h>
#include <errno.h>
#include <imath.h>
#include <stdio.h>
#include <str.h>

/** Simplified capacity parameters */
enum {
	/** Simplified capacity maximum integer digits */
	scap_max_idig = 3,
	/** Simplified capacity maximum significant digits */
	scap_max_sdig = 4
};

static const char *cu_str[] = {
	[cu_byte] = "B",
	[cu_kbyte] = "kB",
	[cu_mbyte] = "MB",
	[cu_gbyte] = "GB",
	[cu_tbyte] = "TB",
	[cu_pbyte] = "PB",
	[cu_ebyte] = "EB",
	[cu_zbyte] = "ZB",
	[cu_ybyte] = "YB"
};

void cap_from_blocks(uint64_t nblocks, size_t block_size, cap_spec_t *cap)
{
	uint64_t tsize;

	tsize = nblocks * block_size;
	cap->m = tsize;
	cap->dp = 0;
	cap->cunit = cu_byte;
}

/** Convert capacity to blocks.
 *
 * If the value of bytes is not integer, it is properly rounded. If the number
 * of bytes is not divisible by the number of blocks, it is rounded
 * up to an integer number of blocks.
 *
 * A capacity value entails precision, i.e. it corresponds to a range
 * of values. @a cvsel selects the value to return. @c cv_nom gives
 * the nominal (middle) value, @c cv_min gives the minimum value
 * and @c cv_max gives the maximum value.
 */
errno_t cap_to_blocks(cap_spec_t *cap, cap_vsel_t cvsel, size_t block_size,
    uint64_t *rblocks)
{
	int exp;
	uint64_t bytes;
	uint64_t f;
	uint64_t adj;
	uint64_t blocks;
	uint64_t rem;
	errno_t rc;

	exp = cap->cunit * 3 - cap->dp;
	if (exp < 0) {
		rc = ipow10_u64(-exp, &f);
		if (rc != EOK)
			return ERANGE;
		bytes = (cap->m + (f / 2)) / f;
		if (bytes * f - (f / 2) != cap->m)
			return ERANGE;
	} else {
		rc = ipow10_u64(exp, &f);
		if (rc != EOK)
			return ERANGE;

		adj = 0;
		switch (cvsel) {
		case cv_nom:
			adj = 0;
			break;
		case cv_min:
			adj = -(f / 2);
			break;
		case cv_max:
			adj = f / 2 - 1;
			break;
		}

		bytes = cap->m * f + adj;
		if ((bytes - adj) / f != cap->m)
			return ERANGE;
	}

	rem = bytes % block_size;
	if ((bytes + rem) < bytes)
		return ERANGE;

	blocks = (bytes + rem) / block_size;

	*rblocks = blocks;
	return EOK;
}

/** Simplify and round capacity to a human-friendly form.
 *
 * Change unit and round the number so that we have at most three integer
 * digits and at most two fractional digits, e.g abc.xy <unit>.
 */
void cap_simplify(cap_spec_t *cap)
{
	uint64_t div;
	uint64_t maxv;
	unsigned sdig;
	unsigned rdig;
	errno_t rc;

	/* Change units so that we have at most @c scap_max_idig integer digits */
	rc = ipow10_u64(scap_max_idig, &maxv);
	assert(rc == EOK);

	rc = ipow10_u64(cap->dp, &div);
	assert(rc == EOK);

	while (cap->m / div >= maxv) {
		++cap->cunit;
		cap->dp += 3;
		div = div * 1000;
	}

	/* Round the number so that we have at most @c scap_max_sdig significant digits */
	sdig = 1 + ilog10_u64(cap->m); /* number of significant digits */
	if (sdig > scap_max_sdig) {
		/* Number of digits to remove */
		rdig = sdig - scap_max_sdig;
		if (rdig > cap->dp)
			rdig = cap->dp;

		rc = ipow10_u64(rdig, &div);
		assert(rc == EOK);

		cap->m = (cap->m + (div / 2)) / div;
		cap->dp -= rdig;
	}
}

errno_t cap_format(cap_spec_t *cap, char **rstr)
{
	errno_t rc;
	int ret;
	const char *sunit;
	uint64_t ipart;
	uint64_t fpart;
	uint64_t div;

	sunit = NULL;

	assert(cap->cunit < CU_LIMIT);

	rc = ipow10_u64(cap->dp, &div);
	if (rc != EOK)
		return rc;

	ipart = cap->m / div;
	fpart = cap->m % div;

	sunit = cu_str[cap->cunit];
	if (cap->dp > 0) {
		ret = asprintf(rstr, "%" PRIu64 ".%0*" PRIu64 " %s", ipart,
		    (int)cap->dp, fpart, sunit);
	} else {
		ret = asprintf(rstr, "%" PRIu64 " %s", ipart, sunit);
	}
	if (ret < 0)
		return ENOMEM;

	return EOK;
}

static errno_t cap_digit_val(char c, int *val)
{
	switch (c) {
	case '0': *val = 0; break;
	case '1': *val = 1; break;
	case '2': *val = 2; break;
	case '3': *val = 3; break;
	case '4': *val = 4; break;
	case '5': *val = 5; break;
	case '6': *val = 6; break;
	case '7': *val = 7; break;
	case '8': *val = 8; break;
	case '9': *val = 9; break;
	default:
		return EINVAL;
	}

	return EOK;
}

errno_t cap_parse(const char *str, cap_spec_t *cap)
{
	const char *eptr;
	const char *p;
	int d;
	int dp;
	unsigned long m;
	int i;

	m = 0;

	eptr = str;
	while (cap_digit_val(*eptr, &d) == EOK) {
		m = m * 10 + d;
		++eptr;
	}

	if (*eptr == '.') {
		++eptr;
		dp = 0;
		while (cap_digit_val(*eptr, &d) == EOK) {
			m = m * 10 + d;
			++dp;
			++eptr;
		}
	} else {
		dp = 0;
	}

	while (*eptr == ' ')
		++eptr;

	if (*eptr == '\0') {
		cap->cunit = cu_byte;
	} else {
		for (i = 0; i < CU_LIMIT; i++) {
			if (str_lcasecmp(eptr, cu_str[i],
			    str_length(cu_str[i])) == 0) {
				p = eptr + str_size(cu_str[i]);
				while (*p == ' ')
					++p;
				if (*p == '\0')
					goto found;
			}
		}

		return EINVAL;
found:
		cap->cunit = i;
	}

	cap->m = m;
	cap->dp = dp;
	return EOK;
}

/** @}
 */
