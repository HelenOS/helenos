/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64_mm
 * @{
 */
/** @file
 * @brief ASIDs related declarations.
 */

#ifndef KERN_arm64_ASID_H_
#define KERN_arm64_ASID_H_

#include <stdint.h>

/*
 * The ASID size is in VMSAv8-64 an implementation defined choice of 8 or 16
 * bits. The actual size can be obtained by reading ID_AA64MMFR0_EL1.ASIDBits
 * but for simplicity, HelenOS currently defaults to 8 bits.
 */
typedef uint8_t asid_t;

#define ASID_MAX_ARCH  255

#endif

/** @}
 */
