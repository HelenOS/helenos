/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32_mm
 * @{
 */
/** @file
 * @ingroup kernel_ia32_mm, kernel_amd64_mm
 */

/*
 * ia32 has no hardware support for address space identifiers.
 * This file is provided to do nop-implementation of mm/asid.h
 * interface.
 */

#ifndef KERN_ia32_ASID_H_
#define KERN_ia32_ASID_H_

#include <stdint.h>

typedef int32_t asid_t;

#define ASID_MAX_ARCH  3

#define asid_get()  (ASID_START + 1)
#define asid_put(asid)

#endif

/** @}
 */
