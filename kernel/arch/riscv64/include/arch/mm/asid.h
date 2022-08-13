/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_ASID_H_
#define KERN_riscv64_ASID_H_

#include <stdint.h>

#define ASID_MAX_ARCH  4096

typedef uint32_t asid_t;

#define asid_get()  (ASID_START + 1)
#define asid_put(asid)

#endif

/** @}
 */
