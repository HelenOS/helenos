/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_TYPEDEFS_H_
#define KERN_TYPEDEFS_H_

#include <arch/types.h>
#include <_bits/errno.h>

typedef void (*function)(void);

typedef uint32_t container_id_t;

typedef int32_t inr_t;

typedef volatile uint8_t ioport8_t;
typedef volatile uint16_t ioport16_t;
typedef volatile uint32_t ioport32_t;

#ifdef __32_BITS__

/** Explicit 64-bit arguments passed to syscalls. */
typedef uint64_t sysarg64_t;

#endif /* __32_BITS__ */

#endif

/** @}
 */
