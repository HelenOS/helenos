/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 * SPDX-FileCopyrightText: 2008 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4v_REGDEF_H_
#define KERN_sparc64_sun4v_REGDEF_H_

#define TSTATE_CWP_MASK  0x1f

#define WSTATE_NORMAL(n)  (n)
#define WSTATE_OTHER(n)   ((n) << 3)

#endif

/** @}
 */
