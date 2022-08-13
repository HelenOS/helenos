/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4v_REGISTER_H_
#define KERN_sparc64_sun4v_REGISTER_H_

#include <arch/regdef.h>
#include <stdint.h>

/** Processor State Register. */
union pstate_reg {
	uint64_t value;
	struct {
		uint64_t : 54;
		unsigned cle : 1;	/**< Current Little Endian. */
		unsigned tle : 1;	/**< Trap Little Endian. */
		unsigned mm : 2;	/**< Memory Model. */
		unsigned : 1;		/**< RED state. */
		unsigned pef : 1;	/**< Enable floating-point. */
		unsigned am : 1;	/**< 32-bit Address Mask. */
		unsigned priv : 1;	/**< Privileged Mode. */
		unsigned ie : 1;	/**< Interrupt Enable. */
		unsigned : 1;
	} __attribute__((packed));
};
typedef union pstate_reg pstate_reg_t;

#endif

/** @}
 */
