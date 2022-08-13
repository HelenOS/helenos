/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 * @ingroup libcarm32
 */

#ifndef _LIBC_arm32_DDI_H_
#define _LIBC_arm32_DDI_H_

#include <ddi.h>

static inline void arch_pio_write_8(ioport8_t *port, uint8_t v)
{
	*port = v;
}

static inline void arch_pio_write_16(ioport16_t *port, uint16_t v)
{
	*port = v;
}

static inline void arch_pio_write_32(ioport32_t *port, uint32_t v)
{
	*port = v;
}

static inline void arch_pio_write_64(ioport64_t *port, uint64_t v)
{
	*port = v;
}

static inline uint8_t arch_pio_read_8(const ioport8_t *port)
{
	return *port;
}

static inline uint16_t arch_pio_read_16(const ioport16_t *port)
{
	return *port;
}

static inline uint32_t arch_pio_read_32(const ioport32_t *port)
{
	return *port;
}

static inline uint64_t arch_pio_read_64(const ioport64_t *port)
{
	return *port;
}

#endif
