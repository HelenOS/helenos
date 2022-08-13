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

#ifndef KERN_sparc64_REGISTER_H_
#define KERN_sparc64_REGISTER_H_

#include <arch/regdef.h>
#include <stdint.h>

/** Version Register. */
union ver_reg {
	uint64_t value;
	struct {
		uint16_t manuf;	/**< Manufacturer code. */
		uint16_t impl;	/**< Implementation code. */
		uint8_t mask;	/**< Mask set revision. */
		unsigned : 8;
		uint8_t maxtl;
		unsigned : 3;
		unsigned maxwin : 5;
	} __attribute__((packed));
};
typedef union ver_reg ver_reg_t;

/** Processor State Register. */
union pstate_reg {
	uint64_t value;
	struct {
		uint64_t : 52;
		unsigned ig : 1;	/**< Interrupt Globals. */
		unsigned mg : 1;	/**< MMU Globals. */
		unsigned cle : 1;	/**< Current Little Endian. */
		unsigned tle : 1;	/**< Trap Little Endian. */
		unsigned mm : 2;	/**< Memory Model. */
		unsigned red : 1;	/**< RED state. */
		unsigned pef : 1;	/**< Enable floating-point. */
		unsigned am : 1;	/**< 32-bit Address Mask. */
		unsigned priv : 1;	/**< Privileged Mode. */
		unsigned ie : 1;	/**< Interrupt Enable. */
		unsigned ag : 1;	/**< Alternate Globals */
	} __attribute__((packed));
};
typedef union pstate_reg pstate_reg_t;

/** TICK Register. */
union tick_reg {
	uint64_t value;
	struct {
		unsigned npt : 1;	/**< Non-privileged Trap enable. */
		uint64_t counter : 63;	/**< Elapsed CPU clck cycle counter. */
	} __attribute__((packed));
};
typedef union tick_reg tick_reg_t;

/** TICK_compare Register. */
union tick_compare_reg {
	uint64_t value;
	struct {
		unsigned int_dis : 1;		/**< TICK_INT interrupt disabled flag. */
		uint64_t tick_cmpr : 63;	/**< Compare value for TICK interrupts. */
	} __attribute__((packed));
};
typedef union tick_compare_reg tick_compare_reg_t;

/** SOFTINT Register. */
union softint_reg {
	uint64_t value;
	struct {
		uint64_t : 47;
		unsigned stick_int : 1;
		unsigned int_level : 15;
		unsigned tick_int : 1;
	} __attribute__((packed));
};
typedef union softint_reg softint_reg_t;

/** Floating-point Registers State Register. */
union fprs_reg {
	uint64_t value;
	struct {
		uint64_t : 61;
		unsigned fef : 1;
		unsigned du : 1;
		unsigned dl : 1;
	} __attribute__((packed));
};
typedef union fprs_reg fprs_reg_t;

#endif

/** @}
 */
