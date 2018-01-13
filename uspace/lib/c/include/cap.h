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

#ifndef LIBC_CAP_H_
#define LIBC_CAP_H_

#include <adt/list.h>
#include <loc.h>
#include <stdint.h>
#include <types/label.h>
#include <types/vol.h>
#include <vbd.h>

/** Capacity unit */
typedef enum {
	cu_byte = 0,
	cu_kbyte,
	cu_mbyte,
	cu_gbyte,
	cu_tbyte,
	cu_pbyte,
	cu_ebyte,
	cu_zbyte,
	cu_ybyte
} cap_unit_t;

/** Which of values within the precision of the capacity */
typedef enum {
	/** The nominal (middling) value */
	cv_nom,
	/** The minimum value */
	cv_min,
	/** The maximum value */
	cv_max
} cap_vsel_t;

#define CU_LIMIT (cu_ybyte + 1)

/** Storage capacity.
 *
 * Storage capacity represents both value and precision.
 * It is a decimal floating point value combined with a decimal
 * capacity unit. There is an integer mantisa @c m which in combination
 * with the number of decimal positions @c dp gives a decimal floating-point
 * number. E.g. for m = 1025 and dp = 2 the number is 10.25. If the unit
 * cunit = cu_kbyte, the capacity is 10.25 kByte, i.e. 10 250 bytes.
 *
 * Note that 1.000 kByte is equivalent to 1000 Byte, but 1 kByte is less
 * precise.
 */
typedef struct {
	/** Mantisa */
	uint64_t m;
	/** Decimal positions */
	unsigned dp;
	/** Capacity unit */
	cap_unit_t cunit;
} cap_spec_t;

extern errno_t cap_format(cap_spec_t *, char **);
extern errno_t cap_parse(const char *, cap_spec_t *);
extern void cap_simplify(cap_spec_t *);
extern void cap_from_blocks(uint64_t, size_t, cap_spec_t *);
extern errno_t cap_to_blocks(cap_spec_t *, cap_vsel_t, size_t, uint64_t *);

#endif

/** @}
 */
