/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_proc
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_THREAD_H_
#define KERN_ia64_THREAD_H_

typedef struct {
} thread_arch_t;

#define thr_constructor_arch(t)
#define thr_destructor_arch(t)
#define thread_create_arch(t, flags) (EOK)

#endif

/** @}
 */
