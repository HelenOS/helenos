/*
 * SPDX-FileCopyrightText: 2007 Pavel Jancik, Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32_mm
 * @{
 */
/** @file
 *  @brief ASIDs related declarations.
 *
 *  ARM CPUs doesn't support ASIDs.
 */

#ifndef KERN_arm32_ASID_H_
#define KERN_arm32_ASID_H_

#include <stdint.h>

#define ASID_MAX_ARCH		3	/* minimal required number */

typedef uint8_t asid_t;

/*
 * This works due to fact that this file is never included alone but only
 * through "generic/include/mm/asid.h" where ASID_START is defined.
 */
#define asid_get()		(ASID_START + 1)

#define asid_put(asid)

#endif

/** @}
 */
