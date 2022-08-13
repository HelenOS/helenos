/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32_mm
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_ASID_H_
#define KERN_mips32_ASID_H_

#include <stdint.h>

#define ASID_MAX_ARCH  255    /* 2^8 - 1 */

typedef uint8_t asid_t;

#endif

/** @}
 */
