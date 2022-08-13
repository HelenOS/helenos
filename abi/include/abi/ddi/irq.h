/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
