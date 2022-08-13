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

#ifndef KERN_HALT_H_
#define KERN_HALT_H_

#include <atomic.h>

extern atomic_bool haltstate;

extern void halt(void) __attribute__((noreturn));

#endif

/** @}
 */
