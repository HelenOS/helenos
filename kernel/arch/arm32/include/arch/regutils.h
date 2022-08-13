/*
 * SPDX-FileCopyrightText: 2007 Petr Stepan
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/**
 * @file
 * @brief Utilities for convenient manipulation with ARM registers.
 */

#ifndef KERN_arm32_REGUTILS_H_
#define KERN_arm32_REGUTILS_H_

#define STATUS_REG_IRQ_DISABLED_BIT  (1 << 7)
#define STATUS_REG_MODE_MASK         0x1f

/* ARM Processor Operation Modes */
enum {
	USER_MODE = 0x10,
	FIQ_MODE = 0x11,
	IRQ_MODE = 0x12,
	SUPERVISOR_MODE = 0x13,
	MONITOR_MODE = 0x16,
	ABORT_MODE = 0x17,
	HYPERVISOR_MODE = 0x1a,
	UNDEFINED_MODE = 0x1b,
	SYSTEM_MODE = 0x1f,
	MODE_MASK = 0x1f,
};
/* [CS]PRS manipulation macros */
#define GEN_STATUS_READ(nm, reg) \
	static inline uint32_t nm## _status_reg_read(void) \
	{ \
		uint32_t retval; \
		\
		asm volatile ( \
			"mrs %[retval], " #reg \
			: [retval] "=r" (retval) \
		); \
		\
		return retval; \
	}

#define GEN_STATUS_WRITE(nm, reg, fieldname, field) \
	static inline void nm## _status_reg_ ##fieldname## _write(uint32_t value) \
	{ \
		asm volatile ( \
			"msr " #reg "_" #field ", %[value]" \
			:: [value] "r" (value) \
		); \
	}

/** Return the value of CPSR (Current Program Status Register). */
GEN_STATUS_READ(current, cpsr);

/** Set control bits of CPSR. */
GEN_STATUS_WRITE(current, cpsr, control, c);

/** Return the value of SPSR (Saved Program Status Register). */
GEN_STATUS_READ(saved, spsr);

#endif

/** @}
 */
