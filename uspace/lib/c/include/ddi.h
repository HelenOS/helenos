/*
 * Copyright (c) 2006 Jakub Jermar
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
/** @file
 */

#ifndef LIBC_DDI_H_
#define LIBC_DDI_H_

#include <stdbool.h>
#include <sys/types.h>
#include <sys/time.h>
#include <abi/ddi/irq.h>
#include <device/hw_res.h>
#include <device/hw_res_parsed.h>
#include <device/pio_window.h>
#include <task.h>

#define DMAMEM_16MiB  ((uintptr_t) UINT64_C(0xffffffffff000000))
#define DMAMEM_4GiB   ((uintptr_t) UINT64_C(0xffffffff00000000))

extern int device_assign_devno(void);

extern int physmem_map(uintptr_t, size_t, unsigned int, void **);
extern int physmem_unmap(void *);

extern int dmamem_map(void *, size_t, unsigned int, unsigned int, uintptr_t *);
extern int dmamem_map_anonymous(size_t, uintptr_t, unsigned int, unsigned int,
    uintptr_t *, void **);
extern int dmamem_unmap(void *, size_t);
extern int dmamem_unmap_anonymous(void *);

extern int pio_enable_range(addr_range_t *, void **);
extern int pio_enable_resource(pio_window_t *, hw_resource_t *, void **);
extern int pio_enable(void *, size_t, void **);
extern int pio_disable(void *, size_t);

typedef void (*trace_fnc)(const volatile void *place, uint32_t val,
    volatile void* base, size_t size, void *data, bool write);

extern int pio_trace_enable(void *, size_t, trace_fnc, void *);
extern void pio_trace_log(const volatile void *, uint32_t val, bool write);
extern void pio_trace_disable(void *);

extern void pio_write_8(ioport8_t *, uint8_t);
extern void pio_write_16(ioport16_t *, uint16_t);
extern void pio_write_32(ioport32_t *, uint32_t);

extern uint8_t pio_read_8(const ioport8_t *);
extern uint16_t pio_read_16(const ioport16_t *);
extern uint32_t pio_read_32(const ioport32_t *);

static inline uint8_t pio_change_8(ioport8_t *reg, uint8_t val, uint8_t mask,
    useconds_t delay)
{
	uint8_t v = pio_read_8(reg);
	udelay(delay);
	pio_write_8(reg, (v & ~mask) | val);
	return v;
}

static inline uint16_t pio_change_16(ioport16_t *reg, uint16_t val,
    uint16_t mask, useconds_t delay)
{
	uint16_t v = pio_read_16(reg);
	udelay(delay);
	pio_write_16(reg, (v & ~mask) | val);
	return v;
}

static inline uint32_t pio_change_32(ioport32_t *reg, uint32_t val,
    uint32_t mask, useconds_t delay)
{
	uint32_t v = pio_read_32(reg);
	udelay(delay);
	pio_write_32(reg, (v & ~mask) | val);
	return v;
}

static inline uint8_t pio_set_8(ioport8_t *r, uint8_t v, useconds_t d)
{
	return pio_change_8(r, v, 0, d);
}
static inline uint16_t pio_set_16(ioport16_t *r, uint16_t v, useconds_t d)
{
	return pio_change_16(r, v, 0, d);
}
static inline uint32_t pio_set_32(ioport32_t *r, uint32_t v, useconds_t d)
{
	return pio_change_32(r, v, 0, d);
}

static inline uint8_t pio_clear_8(ioport8_t *r, uint8_t v, useconds_t d)
{
	return pio_change_8(r, 0, v, d);
}
static inline uint16_t pio_clear_16(ioport16_t *r, uint16_t v, useconds_t d)
{
	return pio_change_16(r, 0, v, d);
}
static inline uint32_t pio_clear_32(ioport32_t *r, uint32_t v, useconds_t d)
{
	return pio_change_32(r, 0, v, d);
}

#endif

/** @}
 */
