/*
 * SPDX-FileCopyrightText: 2011 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup mfs
 * @{
 */

#include <byteorder.h>
#include "mfs.h"

uint16_t
conv16(bool native, uint16_t n)
{
	if (native)
		return n;

	return uint16_t_byteorder_swap(n);
}

uint32_t
conv32(bool native, uint32_t n)
{
	if (native)
		return n;

	return uint32_t_byteorder_swap(n);
}

uint64_t
conv64(bool native, uint64_t n)
{
	if (native)
		return n;

	return uint64_t_byteorder_swap(n);
}

/**
 * @}
 */
