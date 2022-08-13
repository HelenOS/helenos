/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_abs32le_mm
 * @{
 */

#ifndef KERN_abs32le_ASID_H_
#define KERN_abs32le_ASID_H_

#include <stdint.h>

typedef uint32_t asid_t;

#define ASID_MAX_ARCH  3

#define asid_get()      (ASID_START + 1)
#define asid_put(asid)

#endif

/** @}
 */
