/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 * SPDX-FileCopyrightText: 2008 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4v_MMU_H_
#define KERN_sparc64_sun4v_MMU_H_

#define ASI_REAL			0x14	/**< MMU bypass ASI */

#define VA_PRIMARY_CONTEXT_REG		0x8	/**< primary context register VA. */
#define ASI_PRIMARY_CONTEXT_REG		0x21	/**< primary context register ASI. */

#define VA_SECONDARY_CONTEXT_REG	0x10	/**< secondary context register VA. */
#define ASI_SECONDARY_CONTEXT_REG	0x21	/**< secondary context register ASI. */

#endif

/** @}
 */
