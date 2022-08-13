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

#ifndef KERN_sparc64_REGDEF_H_
#define KERN_sparc64_REGDEF_H_

#define PSTATE_IE_BIT	(1 << 1)
#define PSTATE_AM_BIT	(1 << 3)

#define PSTATE_AG_BIT	(1 << 0)
#define PSTATE_IG_BIT	(1 << 11)
#define PSTATE_MG_BIT	(1 << 10)

#define PSTATE_PRIV_BIT	(1 << 2)
#define PSTATE_PEF_BIT	(1 << 4)

#define TSTATE_PSTATE_SHIFT	8
#define TSTATE_PRIV_BIT		(PSTATE_PRIV_BIT << TSTATE_PSTATE_SHIFT)
#define TSTATE_IE_BIT		(PSTATE_IE_BIT << TSTATE_PSTATE_SHIFT)
#define TSTATE_PEF_BIT		(PSTATE_PEF_BIT << TSTATE_PSTATE_SHIFT)

#define TSTATE_CWP_MASK		0x1f

#define WSTATE_NORMAL(n)	(n)
#define WSTATE_OTHER(n)		((n) << 3)

/*
 * The following definitions concern the UPA_CONFIG register on US and the
 * FIREPLANE_CONFIG register on US3.
 */
#define ICBUS_CONFIG_MID_SHIFT    17

#endif

/** @}
 */
