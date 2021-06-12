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

#include <assert.h>
#include <capa.h>
#include <errno.h>
#include <imath.h>
#include <stddef.h>
#include <stdio.h>
#include <str.h>

/** Simplified capacity parameters */
enum {
	/** Simplified capacity maximum integer digits */
	scapa_max_idig = 3,
	/** Simplified capacity maximum significant digits */
	scapa_max_sdig = 4
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

void capa_from_blocks(uint64_t nblocks, size_t block_size, capa_spec_t *capa)
{
	uint64_t tsize;

	tsize = nblocks * block_size;
	capa->m = tsize;
	capa->dp = 0;
	capa->cunit = cu_byte;
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
errno_t capa_to_blocks(capa_spec_t *capa, capa_vsel_t cvsel, size_t block_size,
    uint64_t *rblocks)
{
	int exp;
	uint64_t bytes;
	uint64_t f;
	uint64_t adj;
	uint64_t blocks;
	uint64_t rem;
	errno_t rc;

	exp = capa->cunit * 3 - capa->dp;
	if (exp < 0) {
		rc = ipow10_u64(-exp, &f);
		if (rc != EOK)
			return ERANGE;
		bytes = (capa->m + (f / 2)) / f;
		if (bytes * f - (f / 2) != capa->m)
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

		bytes = capa->m * f + adj;
		if ((bytes - adj) / f != capa->m)
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
void capa_simplify(capa_spec_t *capa)
{
	uint64_t div;
	uint64_t maxv;
	unsigned sdig;
	unsigned rdig;
	errno_t rc;

	/* Change units so that we have at most @c scapa_max_idig integer digits */
	rc = ipow10_u64(scapa_max_idig, &maxv);
	assert(rc == EOK);

	rc = ipow10_u64(capa->dp, &div);
	assert(rc == EOK);

	/* Change units until we have no more than scapa_max_idig integer digits */
	while (capa->m / div >= maxv) {
		++capa->cunit;
		capa->dp += 3;
		div = div * 1000;
	}

	/* Round the number so that we have at most @c scapa_max_sdig significant digits */
	sdig = 1 + ilog10_u64(capa->m); /* number of significant digits */
	if (sdig > scapa_max_sdig) {
		/* Number of digits to remove */
		rdig = sdig - scapa_max_sdig;
		if (rdig > capa->dp)
			rdig = capa->dp;

		rc = ipow10_u64(rdig, &div);
		assert(rc == EOK);

		/* Division with rounding */
		capa->m = (capa->m + (div / 2)) / div;
		capa->dp -= rdig;
	}

	/*
	 * If we rounded up from something like 999.95 to 1000.0,, we still
	 * have more than scapa_max_idig integer digits and need to change
	 * units once more.
	 */
	rc = ipow10_u64(capa->dp, &div);
	assert(rc == EOK);

	if (capa->m / div >= 1000) {
		++capa->cunit;
		capa->dp += 3;

		/*
		 * We now have one more significant digit than we want
		 * so round to one less digits
		 */
		capa->m = (capa->m + 5) / 10;
		--capa->dp;
	}
}

errno_t capa_format(capa_spec_t *capa, char **rstr)
{
	errno_t rc;
	int ret;
	const char *sunit;
	uint64_t ipart;
	uint64_t fpart;
	uint64_t div;

	sunit = NULL;

	assert(capa->cunit < CU_LIMIT);

	rc = ipow10_u64(capa->dp, &div);
	if (rc != EOK)
		return rc;

	ipart = capa->m / div;
	fpart = capa->m % div;

	sunit = cu_str[capa->cunit];
	if (capa->dp > 0) {
		ret = asprintf(rstr, "%" PRIu64 ".%0*" PRIu64 " %s", ipart,
		    (int)capa->dp, fpart, sunit);
	} else {
		ret = asprintf(rstr, "%" PRIu64 " %s", ipart, sunit);
	}

	if (ret < 0)
		return ENOMEM;

	return EOK;
}

static errno_t capa_digit_val(char c, int *val)
{
	switch (c) {
	case '0':
		*val = 0;
		break;
	case '1':
		*val = 1;
		break;
	case '2':
		*val = 2;
		break;
	case '3':
		*val = 3;
		break;
	case '4':
		*val = 4;
		break;
	case '5':
		*val = 5;
		break;
	case '6':
		*val = 6;
		break;
	case '7':
		*val = 7;
		break;
	case '8':
		*val = 8;
		break;
	case '9':
		*val = 9;
		break;
	default:
		return EINVAL;
	}

	return EOK;
}

errno_t capa_parse(const char *str, capa_spec_t *capa)
{
	const char *eptr;
	const char *p;
	int d;
	int dp;
	unsigned long m;
	int i;

	m = 0;

	eptr = str;
	while (capa_digit_val(*eptr, &d) == EOK) {
		m = m * 10 + d;
		++eptr;
	}

	if (*eptr == '.') {
		++eptr;
		dp = 0;
		while (capa_digit_val(*eptr, &d) == EOK) {
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
		capa->cunit = cu_byte;
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
		capa->cunit = i;
	}

	capa->m = m;
	capa->dp = dp;
	return EOK;
}

/** @}
 */
