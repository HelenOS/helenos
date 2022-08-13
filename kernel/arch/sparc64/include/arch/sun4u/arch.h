/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/**
 * @file
 * @brief	Various sun4u-specific macros.
 */

#ifndef KERN_sparc64_sun4u_ARCH_H_
#define KERN_sparc64_sun4u_ARCH_H_

#define ASI_NUCLEUS_QUAD_LDD	0x24	/** ASI for 16-byte atomic loads. */
#define ASI_DCACHE_TAG		0x47	/** ASI D-Cache Tag. */
#define ASI_ICBUS_CONFIG	0x4a	/** ASI of the UPA_CONFIG/FIREPLANE_CONFIG register. */

#endif

/** @}
 */
