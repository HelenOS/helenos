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

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_DDI_IRQ_H_
#define _ABI_DDI_IRQ_H_

#include <stdint.h>

typedef struct {
	uintptr_t base;
	size_t size;
} irq_pio_range_t;

typedef enum {
	/** Read 1 byte from the I/O space.
	 *
	 * *addr(8) -> scratch[dstarg]
	 */
	CMD_PIO_READ_8 = 1,

	/** Read 2 bytes from the I/O space.
	 *
	 * *addr(16) -> scratch[dstarg]
	 */
	CMD_PIO_READ_16,

	/** Read 4 bytes from the I/O space.
	 *
	 * *addr(32) -> scratch[dstarg]
	 */
	CMD_PIO_READ_32,

	/** Write 1 byte to the I/O space.
	 *
	 * value(8) -> *addr
	 */
	CMD_PIO_WRITE_8,

	/** Write 2 bytes to the I/O space.
	 *
	 * value(16) -> *addr
	 */
	CMD_PIO_WRITE_16,

	/** Write 4 bytes to the I/O space.
	 *
	 * value(32) -> *addr
	 */
	CMD_PIO_WRITE_32,

	/** Write 1 byte to the I/O space.
	 *
	 * scratch[srcarg](8) -> *addr
	 */
	CMD_PIO_WRITE_A_8,

	/** Write 2 bytes to the I/O space.
	 *
	 * scratch[srcarg](16) -> *addr
	 */
	CMD_PIO_WRITE_A_16,

	/** Write 4 bytes to the I/O space.
	 *
	 * scratch[srcarg](32) -> *addr
	 */
	CMD_PIO_WRITE_A_32,

	/** Load value.
	 *
	 * value -> scratch[dstarg]
	 */
	CMD_LOAD,

	/** Perform bitwise conjunction.
	 *
	 * scratch[srcarg] & value -> scratch[dstarg]
	 */
	CMD_AND,

	/** Predicate the execution of the following commands.
	 *
	 * if (scratch[srcarg] == 0)
	 *  (skip the following 'value' commands)
	 */
	CMD_PREDICATE,

	/** Accept the interrupt. */
	CMD_ACCEPT,

	/** Decline the interrupt. */
	CMD_DECLINE,
	CMD_LAST
} irq_cmd_type;

typedef struct {
	irq_cmd_type cmd;
	void *addr;
	uint32_t value;
	uintptr_t srcarg;
	uintptr_t dstarg;
} irq_cmd_t;

typedef struct {
	size_t rangecount;
	irq_pio_range_t *ranges;
	size_t cmdcount;
	irq_cmd_t *cmds;
} irq_code_t;

#endif

/** @}
 */
