/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/**
 * @file Storage capacity specification.
 */

#ifndef _LIBC_CAPA_H_
#define _LIBC_CAPA_H_

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

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
} capa_unit_t;

/** Which of values within the precision of the capacity */
typedef enum {
	/** The nominal (middling) value */
	cv_nom,
	/** The minimum value */
	cv_min,
	/** The maximum value */
	cv_max
} capa_vsel_t;

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
	capa_unit_t cunit;
} capa_spec_t;

extern errno_t capa_format(capa_spec_t *, char **);
extern errno_t capa_parse(const char *, capa_spec_t *);
extern void capa_simplify(capa_spec_t *);
extern void capa_from_blocks(uint64_t, size_t, capa_spec_t *);
extern errno_t capa_to_blocks(capa_spec_t *, capa_vsel_t, size_t, uint64_t *);

#endif

/** @}
 */
