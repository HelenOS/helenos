/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief CPU identification.
 */

#ifndef KERN_arm64_CPU_H_
#define KERN_arm64_CPU_H_

/** Struct representing ARM CPU identification. */
typedef struct {
	/** Implementer (vendor) number. */
	uint32_t implementer;

	/** Variant number. */
	uint32_t variant;

	/** Primary part number. */
	uint32_t partnum;

	/** Revision number. */
	uint32_t revision;
} cpu_arch_t;

#endif

/** @}
 */
